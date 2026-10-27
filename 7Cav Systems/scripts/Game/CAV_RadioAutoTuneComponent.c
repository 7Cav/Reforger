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

	protected IEntity m_Owner;
	protected InventoryStorageManagerComponent m_InvMgr;
	protected int m_iPlayerId;

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_Owner = owner;

		if (!Replication.IsServer())
			return;

		GetGame().GetCallqueue().CallLater(DeferredHook, 50, false);
	}

	override protected void OnDelete(IEntity owner)
	{
		ScriptedInventoryStorageManagerComponent invMgr = ScriptedInventoryStorageManagerComponent.Cast(m_InvMgr);
		if (invMgr && invMgr.m_OnItemAddedInvoker)
			invMgr.m_OnItemAddedInvoker.Remove(OnItemAdded);

		UnhookPlayerSpawned();

		if (GetGame())
			GetGame().GetCallqueue().Remove(DeferredHook);

		super.OnDelete(owner);
	}

	protected void DeferredHook()
	{
		m_InvMgr = InventoryStorageManagerComponent.Cast(m_Owner.FindComponent(InventoryStorageManagerComponent));
		ScriptedInventoryStorageManagerComponent invMgr = ScriptedInventoryStorageManagerComponent.Cast(m_InvMgr);
		if (invMgr && invMgr.m_OnItemAddedInvoker)
			invMgr.m_OnItemAddedInvoker.Insert(OnItemAdded);

		// Players are possessed only once their spawn finalizes, after this runs. The player spawned
		// event fires after possession and after a saved arsenal kit is applied, so retune then.
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
			gameMode.GetOnPlayerSpawned().Insert(OnPlayerSpawned);

		RetuneAllRadios();
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
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
			gameMode.GetOnPlayerSpawned().Remove(OnPlayerSpawned);
	}

	//! The player this character belongs to. Not the controlling player: a Game Master possessing an
	//! AI controls it without owning it.
	protected int ResolvePlayerId()
	{
		if (m_iPlayerId > 0)
			return m_iPlayerId;

		return SCR_PossessingManagerComponent.GetPlayerIdFromMainEntity(m_Owner);
	}

	void OnItemAdded(IEntity item, BaseInventoryStorageComponent storage)
	{
		if (!item) return;
		BaseRadioComponent radio = BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent));
		if (radio) RetuneRadioBoth(radio);
	}

	protected void RetuneAllRadios()
	{
		if (!m_InvMgr) return;

		array<IEntity> withComps = {};
		array<typename> want = { BaseRadioComponent };
		m_InvMgr.FindItemsWithComponents(withComps, want);

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
			int gFreq = ResolveGroupFrequency();
			if (chG && gFreq > 0)
				chG.SetFrequency(Math.ClampInt(gFreq, chG.GetMinFrequency(), chG.GetMaxFrequency()));
		}

		if (radio.TransceiversCount() > m_FactionChannelIndex)
		{
			BaseTransceiver chF = radio.GetTransceiver(m_FactionChannelIndex);
			SCR_Faction fac = ResolveFaction();
			if (chF && fac)
			{
				chF.SetFrequency(Math.ClampInt(fac.GetFactionRadioFrequency(), chF.GetMinFrequency(), chF.GetMaxFrequency()));
				if (m_SyncFactionEncryption) radio.SetEncryptionKey(fac.GetFactionRadioEncryptionKey());
			}
		}
	}

	protected int ResolveGroupFrequency()
	{
		int pid = ResolvePlayerId();
		if (pid <= 0)
			return 0;

		SCR_GroupsManagerComponent gm = SCR_GroupsManagerComponent.GetInstance();
		if (!gm)
			return 0;

		SCR_AIGroup group = gm.GetPlayerGroup(pid);
		if (!group)
			return 0;

		return group.GetRadioFrequency();
	}

	//! A player's own faction first: every Cav kit inherits Character_US_Base, so the character itself
	//! can still read "US". Otherwise (AI) the character's own affiliation, never its worn items or vehicle.
	protected SCR_Faction ResolveFaction()
	{
		int pid = ResolvePlayerId();
		if (pid > 0)
		{
			SCR_Faction pf = SCR_Faction.Cast(SCR_FactionManager.SGetPlayerFaction(pid));
			if (pf) return pf;
		}

		FactionAffiliationComponent aff = FactionAffiliationComponent.Cast(m_Owner.FindComponent(FactionAffiliationComponent));
		if (aff)
			return SCR_Faction.Cast(aff.GetAffiliatedFaction());

		return null;
	}
}
