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

		super.OnDelete(owner);
	}

	protected void DeferredHook()
	{
		LocateInvMgr();
		if (m_InvMgr && m_InvMgr.m_OnItemAddedInvoker)
			m_InvMgr.m_OnItemAddedInvoker.Insert(OnItemAdded);

		WaitForFactionThenRetune();
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

		PlayerManager pm = GetGame().GetPlayerManager();
		if (pm)
		{
			int pid = pm.GetPlayerIdFromControlledEntity(m_Owner);
			if (pid != 0)
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

	protected SCR_FactionAffiliationComponent FindAffiliationComp()
	{
		SCR_FactionAffiliationComponent aff = SCR_FactionAffiliationComponent.Cast(m_Owner.FindComponent(SCR_FactionAffiliationComponent));
		if (aff) return aff;

		IEntity p = m_Owner.GetParent();
		while (p)
		{
			aff = SCR_FactionAffiliationComponent.Cast(p.FindComponent(SCR_FactionAffiliationComponent));
			if (aff) return aff;
			p = p.GetParent();
		}

		IEntity c = m_Owner.GetChildren();
		while (c)
		{
			aff = SCR_FactionAffiliationComponent.Cast(c.FindComponent(SCR_FactionAffiliationComponent));
			if (aff) return aff;
			c = c.GetSibling();
		}

		return null;
	}

	protected SCR_Faction ResolveFactionOnce()
	{
		SCR_FactionAffiliationComponent aff = FindAffiliationComp();
		if (aff)
		{
			Faction f = aff.GetAffiliatedFaction();
			if (f)
			{
				SCR_Faction sf = SCR_Faction.Cast(f);
				if (sf) return sf;
			}
		}

		PlayerManager pm = GetGame().GetPlayerManager();
		if (pm)
		{
			int pid = pm.GetPlayerIdFromControlledEntity(m_Owner);
			if (pid != 0)
			{
				SCR_Faction pf = SCR_Faction.Cast(SCR_FactionManager.SGetPlayerFaction(pid));
				if (pf) return pf;
			}
		}
		return null;
	}

	void ForceRetune()
	{
		if (!Replication.IsServer() && !m_RunOnClient) return;
		RetuneAllRadios();
	}
}
