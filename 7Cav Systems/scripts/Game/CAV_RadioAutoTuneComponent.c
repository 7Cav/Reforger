class CAV_GroupRadioSettingsComponentClass : ScriptComponentClass {}
[ComponentEditorProps(category: "Radio", description: "Holds group radio settings (fallback)")]
class CAV_GroupRadioSettingsComponent : ScriptComponent
{
	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Group Frequency (kHz). 0 = unset")]
	protected int m_GroupFrequencyKHz;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Group Encryption Key (optional)")]
	protected string m_GroupEncryptionKey;

	int  GetGroupFrequencyKHz() { return m_GroupFrequencyKHz; }
	bool HasGroupEncryptionKey(out string key) { key = m_GroupEncryptionKey; return key != ""; }
}

class CAV_RadioAutoTuneComponentClass : ScriptComponentClass {}

[ComponentEditorProps(category: "Radio", description: "Auto-tunes radios to group & faction frequencies")]
class CAV_RadioAutoTuneComponent : ScriptComponent
{
	[Attribute(defvalue: "0", uiwidget: UIWidgets.Slider, desc: "Group channel index (Channel 1 = 0)", params: "0 3 1")]
	protected int m_GroupChannelIndex;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.Slider, desc: "Faction channel index (Channel 2 = 1)", params: "0 3 1")]
	protected int m_FactionChannelIndex;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Sync radio encryption key to faction key when faction retunes")]
	protected bool m_SyncFactionEncryption;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Prefer group encryption key (if provided) after group retune")]
	protected bool m_PreferGroupEncryption;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Group Freq Override (kHz). 0 = disabled")]
	protected int m_GroupFreqOverrideKHz;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Run on clients (for local testing)")]
	protected bool m_RunOnClient;

	protected IEntity m_Owner;
	protected ScriptedInventoryStorageManagerComponent m_InvMgr;
	protected InventoryStorageManagerComponent m_InvMgrBase;

	protected int m_FactionRetry = 0;
	protected const int MAX_FACTION_RETRY = 20;

	// Player that took control of this character, set by the game mode's player spawned event.
	protected int m_iPlayerId;
	protected bool m_bSpawnHooked;

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_Owner = owner;

		if (!Replication.IsServer() && !m_RunOnClient)
			return;

		LocateInvMgr();
		GetGame().GetCallqueue().CallLater(DeferredHook, 50, false);
	}

	override protected void OnDelete(IEntity owner)
	{
		if (m_InvMgr && m_InvMgr.m_OnItemAddedInvoker)
			m_InvMgr.m_OnItemAddedInvoker.Remove(OnItemAdded);

		UnhookPlayerSpawned();

		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(DeferredHook);
			GetGame().GetCallqueue().Remove(WaitForFactionThenRetune);
		}

		super.OnDelete(owner);
	}

	protected void DeferredHook()
	{
		if (!m_Owner)
			return;

		LocateInvMgr();
		if (m_InvMgr && m_InvMgr.m_OnItemAddedInvoker)
			m_InvMgr.m_OnItemAddedInvoker.Insert(OnItemAdded);

		// A player character is spawned first and possessed only once the spawn finalizes, which waits
		// for the client to preload the area and can take far longer than the retry window below.
		// The player spawned event fires after possession and after a saved arsenal kit is applied, so
		// it is the point where the player's faction, group and final radios are all known.
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
		{
			gameMode.GetOnPlayerSpawned().Insert(OnPlayerSpawned);
			m_bSpawnHooked = true;
		}

		WaitForFactionThenRetune();
	}

	protected void OnPlayerSpawned(int playerId, IEntity player)
	{
		if (player != m_Owner)
			return;

		m_iPlayerId = playerId;
		UnhookPlayerSpawned();
		RetuneAllRadios();
	}

	protected void UnhookPlayerSpawned()
	{
		if (!m_bSpawnHooked)
			return;

		m_bSpawnHooked = false;
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
			gameMode.GetOnPlayerSpawned().Remove(OnPlayerSpawned);
	}

	protected int ResolvePlayerId()
	{
		if (m_iPlayerId > 0)
			return m_iPlayerId;

		if (!m_Owner)
			return 0;

		// The player this character belongs to, not whoever controls it right now: a Game Master
		// possessing an AI controls it without owning it, and must not have their faction or group
		// tuned into the AI's radios.
		return SCR_PossessingManagerComponent.GetPlayerIdFromMainEntity(m_Owner);
	}

	protected void LocateInvMgrOn(IEntity ent)
	{
		if (!ent) return;
		if (!m_InvMgr) m_InvMgr = ScriptedInventoryStorageManagerComponent.Cast(ent.FindComponent(ScriptedInventoryStorageManagerComponent));
	}

	protected void ScanChildrenForInvMgr(IEntity ent)
	{
		IEntity c = ent.GetChildren();
		while (!m_InvMgr && c)
		{
			LocateInvMgrOn(c);
			c = c.GetSibling();
		}
	}

	protected void LocateInvMgr()
	{
		if (!m_Owner) return;
		LocateInvMgrOn(m_Owner);
		if (!m_InvMgr) ScanChildrenForInvMgr(m_Owner);
		m_InvMgrBase = InventoryStorageManagerComponent.Cast(m_InvMgr);
	}

	void OnItemAdded(IEntity item, BaseInventoryStorageComponent storage)
	{
		if (!item) return;
		BaseRadioComponent radio = BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent));
		if (radio) RetuneRadioBoth(radio);
	}

	protected void WaitForFactionThenRetune()
	{
		if (!m_Owner)
			return;

		SCR_Faction fac = ResolveFactionOnce();
		if (fac)
		{
			RetuneAllRadios();
			return;
		}

		if (m_FactionRetry < MAX_FACTION_RETRY)
		{
			m_FactionRetry++;
			GetGame().GetCallqueue().CallLater(WaitForFactionThenRetune, 200, false);
		}
	}

	protected void RetuneAllRadios()
	{
		if (!m_InvMgrBase) return;

		array<IEntity> withComps = {};
		array<typename> want = { BaseRadioComponent };
		m_InvMgrBase.FindItemsWithComponents(withComps, want);

		foreach (IEntity it : withComps)
		{
			BaseRadioComponent radio = BaseRadioComponent.Cast(it.FindComponent(BaseRadioComponent));
			if (radio) RetuneRadioBoth(radio);
		}
	}

	protected void RetuneRadioBoth(BaseRadioComponent radio)
	{
		if (!radio) return;

		if (radio.TransceiversCount() > m_GroupChannelIndex)
		{
			BaseTransceiver chG = radio.GetTransceiver(m_GroupChannelIndex);
			if (chG)
			{
				int gFreq; string gKey;
				if (ResolveGroupRadio(gFreq, gKey))
				{
					int gMin = chG.GetMinFrequency();
					int gMax = chG.GetMaxFrequency();
					if (gFreq < gMin) gFreq = gMin;
					if (gFreq > gMax) gFreq = gMax;
					chG.SetFrequency(gFreq);
					if (m_PreferGroupEncryption && gKey != "") radio.SetEncryptionKey(gKey);
				}
			}
		}

		if (radio.TransceiversCount() > m_FactionChannelIndex)
		{
			BaseTransceiver chF = radio.GetTransceiver(m_FactionChannelIndex);
			if (chF)
			{
				SCR_Faction fac = ResolveFactionOnce();
				if (fac)
				{
					int fFreq = fac.GetFactionRadioFrequency();
					int fMin = chF.GetMinFrequency();
					int fMax = chF.GetMaxFrequency();
					if (fFreq < fMin) fFreq = fMin;
					if (fFreq > fMax) fFreq = fMax;
					chF.SetFrequency(fFreq);
					if (m_SyncFactionEncryption) radio.SetEncryptionKey(fac.GetFactionRadioEncryptionKey());
				}
			}
		}
	}

	protected bool ResolveGroupRadio(out int freqKHz, out string key)
	{
		if (m_GroupFreqOverrideKHz > 0)
		{
			freqKHz = m_GroupFreqOverrideKHz; key = ""; return true;
		}

		int pid = ResolvePlayerId();
		if (pid > 0)
		{
			SCR_GroupsManagerComponent gm = SCR_GroupsManagerComponent.GetInstance();
			if (gm)
			{
				SCR_AIGroup group = gm.GetPlayerGroup(pid);
				if (group)
				{
					int liveFreq = group.GetRadioFrequency();
					if (liveFreq > 0) { freqKHz = liveFreq; key = ""; return true; }
				}
			}
		}

		CAV_GroupRadioSettingsComponent g = FindGroupSettings();
		if (g && g.GetGroupFrequencyKHz() > 0)
		{
			freqKHz = g.GetGroupFrequencyKHz();
			if (!g.HasGroupEncryptionKey(key)) key = "";
			return true;
		}

		freqKHz = 0; key = ""; return false;
	}

	protected CAV_GroupRadioSettingsComponent FindGroupSettings()
	{
		if (!m_Owner)
			return null;

		CAV_GroupRadioSettingsComponent g = CAV_GroupRadioSettingsComponent.Cast(m_Owner.FindComponent(CAV_GroupRadioSettingsComponent));
		if (g) return g;

		IEntity p = m_Owner.GetParent();
		while (p)
		{
			g = CAV_GroupRadioSettingsComponent.Cast(p.FindComponent(CAV_GroupRadioSettingsComponent));
			if (g) return g;
			p = p.GetParent();
		}

		IEntity c = m_Owner.GetChildren();
		while (c)
		{
			g = CAV_GroupRadioSettingsComponent.Cast(c.FindComponent(CAV_GroupRadioSettingsComponent));
			if (g) return g;
			c = c.GetSibling();
		}
		return null;
	}

	protected SCR_Faction ResolveFactionOnce()
	{
		if (!m_Owner)
			return null;

		// A player's own faction is authoritative. The character's affiliation is not: every Cav kit
		// inherits Character_US_Base, so until the game updates it the character still reads "US".
		int pid = ResolvePlayerId();
		if (pid > 0)
		{
			SCR_Faction pf = SCR_Faction.Cast(SCR_FactionManager.SGetPlayerFaction(pid));
			if (pf) return pf;
		}

		// AI and not yet possessed characters. Characters carry SCR_CharacterFactionAffiliationComponent,
		// which is not an SCR_FactionAffiliationComponent, so look up the shared base class. Only on the
		// character itself: its children are worn items and its parent is any vehicle it sits in, and
		// either can carry a different faction.
		FactionAffiliationComponent aff = FactionAffiliationComponent.Cast(m_Owner.FindComponent(FactionAffiliationComponent));
		if (aff)
			return SCR_Faction.Cast(aff.GetAffiliatedFaction());

		return null;
	}

	void ForceRetune()
	{
		if (!Replication.IsServer() && !m_RunOnClient) return;
		RetuneAllRadios();
	}
}
