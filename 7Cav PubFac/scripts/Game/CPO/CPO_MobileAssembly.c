//------------------------------------------------------------------------------------------------
//! Mobile assembly for the 7Cav command truck.
//!
//! Vanilla command trucks only become spawn points under Conflict: SCR_CampaignMobileAssemblyComponent,
//! its deploy action and even SCR_CampaignMobileAssemblyStandaloneComponent all start with
//! SCR_GameModeCampaign.GetInstance() and return when it is null, so on any other game mode the whole
//! chain is inert. This is the game-mode-agnostic replacement: a deploy action at the trunk creates a
//! faction-keyed spawn point entity, a pack action removes it, and a watchdog packs up automatically
//! when the truck drives off or is destroyed.
//!
//! The spawn point gets its own entity class so game modes can recognise truck spawns WITHOUT a
//! compile-time dependency on this mod: match the type name "CPO_MobileSpawnPoint" as a string.

class CPO_MobileSpawnPointClass : SCR_SpawnPointClass
{
}

//------------------------------------------------------------------------------------------------
//! Spawn point deployed from a command truck. Deliberately adds nothing over SCR_SpawnPoint; the
//! distinct type is the discriminator for cost or denial policy in game modes.
//!
//! THE CLASS NAME IS A CROSS-MOD CONTRACT: CPO Frontline (2.1.1+) string-matches
//! "CPO_MobileSpawnPoint" for its command_truck_spawn_cost_multiplier. Renaming this class, or
//! deploying a subclass in its place, silently degrades that multiplier to 1 on their side.
//! Coordinate with the Frontline session before either.
class CPO_MobileSpawnPoint : SCR_SpawnPoint
{
}

class CPO_MobileAssemblyComponentClass : ScriptComponentClass
{
}

//------------------------------------------------------------------------------------------------
//! Sits on the command box of the 7Cav command truck. Owns the deployed spawn point's lifecycle:
//! the deploy and pack user actions call in here, and a 2 s watchdog packs the assembly when the
//! truck is destroyed or moves off the deploy position. The spawn point is a separate top-level
//! entity, not a child of the truck, so it is never captured by vehicle persistence: after a
//! restart the truck always comes back packed.
class CPO_MobileAssemblyComponent : ScriptComponent
{
	[Attribute("{6C3EF4C7A1D26402}Prefabs/Systems/MobileAssembly/CPO_MobileSpawnPoint.et", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", desc: "Spawn point prefab created on deployment")]
	protected ResourceName m_sSpawnPointPrefab;

	[Attribute("7Cav", desc: "Only players of this faction see the deploy and pack actions")]
	protected FactionKey m_FactionKey;

	// The truck settling on its suspension moves its origin slightly; anything past this is driving.
	protected const float MAX_DRIFT_METERS = 3;
	protected const int WATCHDOG_PERIOD_MS = 2000;

	[RplProp(onRplName: "OnDeployedChanged")]
	protected bool m_bDeployed;

	protected IEntity m_SpawnPoint;
	protected vector m_vDeployPos;

	//------------------------------------------------------------------------------------------------
	bool IsDeployed()
	{
		return m_bDeployed;
	}

	//------------------------------------------------------------------------------------------------
	FactionKey GetFactionKey()
	{
		return m_FactionKey;
	}

	//------------------------------------------------------------------------------------------------
	//! The component lives on the slotted command box; physics, damage and replication live on the
	//! truck root.
	IEntity GetVehicleRoot()
	{
		IEntity root = GetOwner();
		while (root.GetParent())
			root = root.GetParent();

		return root;
	}

	//------------------------------------------------------------------------------------------------
	bool IsVehicleDestroyed()
	{
		DamageManagerComponent damageManager = DamageManagerComponent.Cast(GetVehicleRoot().FindComponent(DamageManagerComponent));
		return damageManager && damageManager.GetState() == EDamageState.DESTROYED;
	}

	//------------------------------------------------------------------------------------------------
	bool IsVehicleMoving()
	{
		Physics physics = GetVehicleRoot().GetPhysics();
		if (!physics)
			return false;

		vector velocity = physics.GetVelocity();
		velocity[1] = 0;
		return velocity.LengthSq() > 0.01;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsAuthority()
	{
		RplComponent rplComponent = RplComponent.Cast(GetVehicleRoot().FindComponent(RplComponent));
		return !rplComponent || !rplComponent.IsProxy();
	}

	//------------------------------------------------------------------------------------------------
	//! Deploy and dismantle feedback, vanilla's own MHQ sounds. Runs on the server directly from
	//! Deploy and Pack, and on clients through the replication callback on m_bDeployed; the events
	//! live in the shared M923A1 sound setup, the same one vanilla's Conflict MHQ plays them from.
	protected void OnDeployedChanged()
	{
		if (System.IsConsoleApp())
			return;

		SCR_VehicleSoundComponent soundComponent = SCR_VehicleSoundComponent.Cast(GetVehicleRoot().FindComponent(SCR_VehicleSoundComponent));
		if (!soundComponent)
			return;

		if (m_bDeployed)
			soundComponent.SoundEvent(SCR_SoundEvent.SOUND_MHQ_DEPLOY);
		else
			soundComponent.SoundEvent(SCR_SoundEvent.SOUND_MHQ_DISMANTLE);
	}

	//------------------------------------------------------------------------------------------------
	//! Server only; the actions do not broadcast, so client-side calls never reach this.
	void Deploy()
	{
		if (!IsAuthority() || m_bDeployed)
			return;

		Resource resource = Resource.Load(m_sSpawnPointPrefab);
		if (!resource.IsValid())
			return;

		IEntity root = GetVehicleRoot();
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		root.GetTransform(params.Transform);

		m_SpawnPoint = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		if (!m_SpawnPoint)
			return;

		m_vDeployPos = root.GetOrigin();
		m_bDeployed = true;
		Replication.BumpMe();
		OnDeployedChanged();

		GetGame().GetCallqueue().CallLater(Watchdog, WATCHDOG_PERIOD_MS, true);
	}

	//------------------------------------------------------------------------------------------------
	void Pack()
	{
		if (!IsAuthority() || !m_bDeployed)
			return;

		GetGame().GetCallqueue().Remove(Watchdog);

		if (m_SpawnPoint)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_SpawnPoint);
			m_SpawnPoint = null;
		}

		m_bDeployed = false;
		Replication.BumpMe();
		OnDeployedChanged();
	}

	//------------------------------------------------------------------------------------------------
	protected void Watchdog()
	{
		if (!m_bDeployed)
			return;

		if (IsVehicleDestroyed() || vector.Distance(GetVehicleRoot().GetOrigin(), m_vDeployPos) > MAX_DRIFT_METERS)
			Pack();
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(Watchdog);

		if (m_SpawnPoint)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_SpawnPoint);
			m_SpawnPoint = null;
		}

		super.OnDelete(owner);
	}
}

//------------------------------------------------------------------------------------------------
//! Trunk action that establishes the spawn point. Faction gating sits in CanBeShownScript only,
//! like vanilla's deploy action: CanBePerformedScript also runs on the dedicated server, where
//! there is no local player faction to compare against.
class CPO_DeployMobileSpawnUserAction : ScriptedUserAction
{
	protected CPO_MobileAssemblyComponent m_Assembly;

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		if (pOwnerEntity)
			m_Assembly = CPO_MobileAssemblyComponent.Cast(pOwnerEntity.FindComponent(CPO_MobileAssemblyComponent));
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_Assembly || m_Assembly.IsDeployed() || m_Assembly.IsVehicleDestroyed())
			return false;

		Faction faction = SCR_FactionManager.SGetLocalPlayerFaction();
		return faction && faction.GetFactionKey() == m_Assembly.GetFactionKey();
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (!m_Assembly || m_Assembly.IsDeployed() || m_Assembly.IsVehicleDestroyed())
			return false;

		if (m_Assembly.IsVehicleMoving())
		{
			SetCannotPerformReason("Stop the vehicle first");
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (m_Assembly)
			m_Assembly.Deploy();
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false;
	}
}

//------------------------------------------------------------------------------------------------
//! Trunk action that packs the spawn point up again.
class CPO_PackMobileSpawnUserAction : ScriptedUserAction
{
	protected CPO_MobileAssemblyComponent m_Assembly;

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		if (pOwnerEntity)
			m_Assembly = CPO_MobileAssemblyComponent.Cast(pOwnerEntity.FindComponent(CPO_MobileAssemblyComponent));
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_Assembly || !m_Assembly.IsDeployed())
			return false;

		Faction faction = SCR_FactionManager.SGetLocalPlayerFaction();
		return faction && faction.GetFactionKey() == m_Assembly.GetFactionKey();
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		return m_Assembly && m_Assembly.IsDeployed();
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (m_Assembly)
			m_Assembly.Pack();
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false;
	}
}

//------------------------------------------------------------------------------------------------
class CPO_MobileSpawnLockComponentClass : SCR_BaseLockComponentClass
{
}

//------------------------------------------------------------------------------------------------
//! Vanilla's Conflict MHQ pattern for the 7Cav command truck: while the spawn point is deployed,
//! every seat refuses entry with vanilla's own mobile-assembly message, so nobody unknowingly
//! drives the squad's spawn away; pack it up first. SCR_GetInUserAction consults the first
//! SCR_BaseLockComponent on the vehicle, so like vanilla's MHQ prefab the truck disables the
//! inherited plain lock instance and carries this one instead; spawn protection still answers
//! through the base class. Entry only: a driver already seated can still pull away, and the
//! drift watchdog packs the point then, same as vanilla's velocity check.
class CPO_MobileSpawnLockComponent : SCR_BaseLockComponent
{
	protected CPO_MobileAssemblyComponent m_Assembly;

	//------------------------------------------------------------------------------------------------
	//! The assembly lives on the slotted command box, the lock on the truck root; walk the slots
	//! the way vanilla's SCR_CampaignMHQLockComponent does.
	protected CPO_MobileAssemblyComponent GetAssembly()
	{
		if (m_Assembly)
			return m_Assembly;

		SlotManagerComponent slotManager = SlotManagerComponent.Cast(GetOwner().FindComponent(SlotManagerComponent));
		if (!slotManager)
			return null;

		array<EntitySlotInfo> slots = {};
		slotManager.GetSlotInfos(slots);

		foreach (EntitySlotInfo slot : slots)
		{
			if (!slot)
				continue;

			IEntity attached = slot.GetAttachedEntity();
			if (!attached)
				continue;

			m_Assembly = CPO_MobileAssemblyComponent.Cast(attached.FindComponent(CPO_MobileAssemblyComponent));
			if (m_Assembly)
				break;
		}

		return m_Assembly;
	}

	//------------------------------------------------------------------------------------------------
	override LocalizedString GetCannotPerformReason(IEntity user)
	{
		CPO_MobileAssemblyComponent assembly = GetAssembly();
		if (assembly && assembly.IsDeployed())
			return "#AR-Campaign_MobileAssemblyDeployed-UC";

		return super.GetCannotPerformReason(user);
	}

	//------------------------------------------------------------------------------------------------
	override bool IsLocked(IEntity user, BaseCompartmentSlot compartmentSlot)
	{
		if (super.IsLocked(user, compartmentSlot))
			return true;

		CPO_MobileAssemblyComponent assembly = GetAssembly();
		if (assembly)
			return assembly.IsDeployed();

		return false;
	}
}
