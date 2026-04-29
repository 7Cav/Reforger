class ParachuteComponentExtendedClass : ParachuteComponentClass {}
class ParachuteComponentExtended : ParachuteComponent
{
	// Delete timing (20 retries * 200ms = 4s; allows async eject to complete)
	protected static const int PARACHUTE_DELETE_MAX_RETRIES = 20;
	protected static const int PARACHUTE_DELETE_POLL_INTERVAL_MS = 200;
	protected static const int SETUP_DELAY_MS = 50;
	protected static const int DELETE_AFTER_EJECT_DELAY_MS = 200;

	// Retry limits for owner client resolve/enter
	protected static const int RESOLVE_CHUTE_MAX_RETRIES = 20;
	protected static const int ENTER_CHUTE_MAX_RETRIES = 20;

	protected static const float CHUTE_EXISTENCE_CHECK_INTERVAL_SEC = 0.5;

	[Attribute("2.0", UIWidgets.Slider, "Deploy invincibility duration (s)", "0.5 10 0.5", category : "Landing")]
	protected float m_fDeployInvincibilityDuration = 1.0;

	// Collapse timing: delay before deleting chute after player exits. Must match or exceed the visual collapse
	// on ParachuteDeployedEntityExtended (fall speed, tilt). Entity collapse params are on the chute prefab.
	[Attribute("3000", UIWidgets.Slider, "Collapse duration before delete (ms)", "500 5000 100", category : "Landing")]
	protected int m_iCollapseDurationMs = 3000;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (SCR_Global.IsEditMode())
			return;
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}

	protected float m_fChuteExistenceCheckAccumulator = 0;

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		if (!IsAuthority())
			return;
		if (!m_bParachuteDeployed)
			return;
		m_fChuteExistenceCheckAccumulator += timeSlice;
		if (m_fChuteExistenceCheckAccumulator < CHUTE_EXISTENCE_CHECK_INTERVAL_SEC)
			return;
		m_fChuteExistenceCheckAccumulator = 0;
		IEntity chute = m_DeployedParachute;
		if (!chute)
			return;
		if (ParachuteHelperFunctions.IsEntityValid(chute))
			return;
		ClearParachuteExitState();
		if (m_ChutePendingDelete == chute)
			m_ChutePendingDelete = null;
	}

	protected void SetDeployInvincibility(IEntity pilot, bool invincible)
	{
		ParachuteHelperFunctions.SetEntityDamageHandling(pilot, !invincible);
	}

	protected void EnableDeployInvincibility(IEntity pilot)
	{
		SetDeployInvincibility(pilot, true);
	}

	protected void RestoreDeployInvincibility(IEntity pilot)
	{
		SetDeployInvincibility(pilot, false);
	}

	protected void CleanupFailedDeploy(IEntity chute, IEntity pilot)
	{
		if (chute)
			DeleteParachuteEntity(chute);
		ClearParachuteExitState();
		RestoreDeployInvincibility(pilot);
	}

	protected void ApplyLandingDamage(float velocityAtExit)
	{
		if (velocityAtExit < m_fHardLandingVelocity)
			return;
		if (!m_PlayerController)
			return;

		IEntity pilot = SCR_ChimeraCharacter.Cast(m_PlayerController.GetMainEntity());
		if (pilot)
			RestoreDeployInvincibility(pilot);
		if (velocityAtExit < m_fDeathLandingVelocity)
			BreakLegs_Server();
		else
			KillPlayer_Server();
	}

	// Returns true on success. On false, chute is already cleaned up; caller restores invincibility only if it was enabled.
	protected bool TryDeployChuteForPilot(IEntity pilot, ParachuteItemComponent item, ParachuteDeployedEntity oldChuteToDelete)
	{
		if (!IsAuthority())
			return false;

		if (!pilot || !ParachuteHelperFunctions.IsEntityValid(pilot))
			return false;

		if (!item)
			return false;

		ResourceName prefab = item.GetParachutePrefab();
		if (prefab == "")
			return false;

		if (!GetGame())
			return false;

		EntitySpawnParams sp = new EntitySpawnParams;
		sp.TransformMode = ETransformMode.WORLD;
		pilot.GetWorldTransform(sp.Transform);

		IEntity spawned = GetGame().SpawnEntityPrefabEx(prefab, false, GetGame().GetWorld(), sp);
		ParachuteDeployedEntity chute = ParachuteDeployedEntity.Cast(spawned);
		if (!chute)
			return false;

		BaseCompartmentSlot pilotSlot = ParachuteHelperFunctions.FindCargoSlotOnEntity(chute);
		if (!pilotSlot)
		{
			DeleteParachuteEntity(chute);
			return false;
		}

		if (!SCR_CompartmentAccessComponent.Cast(pilot.FindComponent(SCR_CompartmentAccessComponent)))
		{
			DeleteParachuteEntity(chute);
			return false;
		}

		EnableDeployInvincibility(pilot);

		Physics physics = pilot.GetPhysics();
		if (!physics)
		{
			CleanupFailedDeploy(chute, pilot);
			return false;
		}

		if (!SeatPilotInChuteAndSetup(pilot, chute, pilotSlot, item, physics.GetVelocity()))
		{
			CleanupFailedDeploy(chute, pilot);
			return false;
		}

		item.SetParachuteUsed_Server();

		if (oldChuteToDelete && ParachuteHelperFunctions.IsEntityValid(oldChuteToDelete))
			ScheduleChuteDeleteWithPolling(oldChuteToDelete, false);

		return true;
	}

	void PollUntilEmptyThenDeleteChute(IEntity chute, int retryCount, bool clearState = true)
	{
		if (!chute)
		{
			if (clearState)
				ClearParachuteExitState();
			return;
		}
		if (!ParachuteHelperFunctions.IsEntityValid(chute))
		{
			if (m_ChutePendingDelete == chute)
				m_ChutePendingDelete = null;
			if (clearState)
				ClearParachuteExitState();
			return;
		}
		if (retryCount >= PARACHUTE_DELETE_MAX_RETRIES)
		{
			DeleteParachuteEntity(chute);
			if (clearState)
				ClearParachuteExitState();
			return;
		}
		if (!GetGame())
		{
			m_ChutePendingDelete = null;
			if (clearState)
				ClearParachuteExitState();
			return;
		}

		if (ParachuteHelperFunctions.IsChuteCompartmentEmpty(chute))
		{
			GetGame().GetCallqueue().CallLater(DeleteParachuteEntity, m_iCollapseDurationMs, false, chute);
			if (clearState)
				ClearParachuteExitState();
			return;
		}
		GetGame().GetCallqueue().CallLater(PollUntilEmptyThenDeleteChute, PARACHUTE_DELETE_POLL_INTERVAL_MS, true, chute, retryCount + 1, clearState);
	}

	void ClearParachuteExitState()
	{
		m_DeployedParachute = null;
		m_bParachuteDeployed = false;
		m_DeployedChuteId = RplId.Invalid();
		m_iChuteSlotId = -1;
		m_ParachuteItem = null;
		m_fChuteExistenceCheckAccumulator = 0;
		Replication.BumpMe();
	}

	void TryEjectOccupantFromChute(IEntity chute)
	{
		ParachuteHelperFunctions.EjectOccupantFromSlot(ParachuteHelperFunctions.FindCargoSlotOnEntity(chute));
	}

	// Tracks chute currently in delete flow to avoid duplicate scheduling.
	protected IEntity m_ChutePendingDelete;

	// Ejects the occupant, clears state (if requested), and schedules polling until the compartment is empty before deleting the chute.
	void ScheduleChuteDeleteWithPolling(IEntity chute, bool clearState = true)
	{
		if (!ParachuteHelperFunctions.IsEntityValid(chute))
			return;
		if (chute == m_ChutePendingDelete)
			return;
		m_ChutePendingDelete = chute;
		TryEjectOccupantFromChute(chute);
		if (clearState)
			ClearParachuteExitState();
		if (GetGame())
			GetGame().GetCallqueue().CallLater(PollUntilEmptyThenDeleteChute, PARACHUTE_DELETE_POLL_INTERVAL_MS, true, chute, 0, clearState);
	}

	bool SeatPilotInChuteAndSetup(IEntity pilot, ParachuteDeployedEntity chute, BaseCompartmentSlot pilotSlot, ParachuteItemComponent item, vector deployVel)
	{
		if (!GetGame())
			return false;

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(pilot.FindComponent(SCR_CompartmentAccessComponent));
		if (!access)
			return false;

		m_vDeployVelocity = deployVel;
		m_DeployedParachute = chute;
		m_ParachuteItem = item;
		m_DeployedChuteId = chute.GetRplId();
		m_iChuteSlotId = pilotSlot.GetCompartmentSlotID();
		m_bParachuteDeployed = true;

		GiveChuteOwnershipToController(chute);

		chute.InitializePilot(pilot, access, deployVel);

		ParachuteDeployedEntityExtended chuteExt = ParachuteDeployedEntityExtended.Cast(chute);
		if (chuteExt)
			chuteExt.StartDeployInvincibility(m_fDeployInvincibilityDuration);

		access.GetInVehicle(chute, pilotSlot, true, 0, ECloseDoorAfterActions.INVALID, true);

		Replication.BumpMe();
		GetGame().GetCallqueue().CallLater(Do_SetupDeployedChute_Owner, SETUP_DELAY_MS, false, m_DeployedChuteId, m_iChuteSlotId, deployVel);
		// Kept in sync with chute's EndDeployInvincibility; both use m_fDeployInvincibilityDuration
		GetGame().GetCallqueue().CallLater(RestoreDeployInvincibility, (int)(m_fDeployInvincibilityDuration * 1000), false, pilot);
		return true;
	}

	override void OnDestroyed(Instigator killer, IEntity killerEntity)
	{
		if (!IsAuthority())
			return;

		if (!m_DeployedParachute)
			return;

		ScheduleChuteDeleteWithPolling(m_DeployedParachute);
	}

	override void RpcAskDeployParachute()
	{
		if (m_bParachuteDeployed)
			return;

		IEntity pilot = GetPilotEntity();
		if (!pilot)
			return;

		ParachuteItemComponent item = ResolveParachuteItem_Server(pilot);
		if (!item)
			return;

		if (!MayDeployParachute_Internal(pilot, item))
			return;

		TryDeployChuteForPilot(pilot, item, null);
	}

	override void Do_SetupDeployedChute_Owner(RplId chuteId, int slotId, vector deployVel)
	{
		m_iResolveChuteTries = 0;
		Rpc(RpcDo_SetupDeployedChute_Owner, chuteId, slotId, deployVel);
	}

	override void OnRep_DeployState()
	{
		m_iResolveChuteTries = 0;
		super.OnRep_DeployState();
	}

	protected int m_iResolveChuteTries;
	protected int m_iEnterChuteTries;

	override protected void RetryResolve_Owner()
	{
		m_iResolveChuteTries++;
		if (m_iResolveChuteTries >= RESOLVE_CHUTE_MAX_RETRIES)
		{
			ClearParachuteExitState();
			return;
		}
		super.RetryResolve_Owner();
	}

	override protected void RetryEnterChute_Owner()
	{
		m_iEnterChuteTries++;
		if (m_iEnterChuteTries >= ENTER_CHUTE_MAX_RETRIES)
		{
			ClearParachuteExitState();
			return;
		}
		super.RetryEnterChute_Owner();
	}

	override protected void TryResolveChute_Owner()
	{
		if (!m_bParachuteDeployed)
			return;

		if (m_DeployedParachute)
			return;

		if (m_DeployedChuteId == RplId.Invalid())
			return;

		Managed instance = Replication.FindItem(m_DeployedChuteId);
		RplComponent rplComp = RplComponent.Cast(instance);
		if (!rplComp)
		{
			RetryResolve_Owner();
			return;
		}

		ParachuteDeployedEntity chute = ParachuteDeployedEntity.Cast(rplComp.GetEntity());
		if (!chute)
		{
			RetryResolve_Owner();
			return;
		}

		m_DeployedParachute = chute;

		IEntity pilot = GetPilotEntity();
		if (pilot)
			m_DeployedParachute.InitializePilot(pilot, m_CompartmentAccess, m_vDeployVelocity);

		m_iEnterChuteTries = 0;
		TryEnterChute_Owner();
	}

	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		SCR_ChimeraCharacter pilot = SCR_ChimeraCharacter.Cast(to);
		if (!pilot)
		{
			m_PilotEntity = null;
			m_CompartmentAccess = null;
			m_ParachuteItem = null;
			if (IsAuthority() && m_bParachuteDeployed && m_DeployedParachute)
				ScheduleChuteDeleteWithPolling(m_DeployedParachute);
			return;
		}

		m_PilotEntity = to;
		m_CompartmentAccess = SCR_CompartmentAccessComponent.Cast(to.FindComponent(SCR_CompartmentAccessComponent));
		RefreshParachuteItemFromInventory(to);

		if (!IsAuthority() || !m_bParachuteDeployed || !m_DeployedParachute)
			return;

		ParachuteDeployedEntityExtended chuteExt = ParachuteDeployedEntityExtended.Cast(m_DeployedParachute);
		if (chuteExt && chuteExt.IsDeployInvincibilityActive())
			return;

		ScheduleChuteDeleteWithPolling(m_DeployedParachute);
	}

	void RespawnChuteForDisconnectedPilot(ParachuteDeployedEntity oldChute, IEntity pilot)
	{
		if (!oldChute || !pilot)
			return;

		if (!ParachuteHelperFunctions.IsEntityValid(oldChute) || !ParachuteHelperFunctions.IsEntityValid(pilot))
			return;

		SCR_ChimeraCharacter pawn = SCR_ChimeraCharacter.Cast(pilot);
		if (!pawn || pawn.IsInVehicle())
			return;

		if (ParachuteHelperFunctions.GetHeightAboveTerrain(pilot.GetOrigin()) < m_fMinimumAltitude)
			return;

		// Pilot never attached or exited during invincibility - reset the item we used so we can deploy again
		ParachuteItemComponentExtended itemExt = ParachuteItemComponentExtended.Cast(m_ParachuteItem);
		if (itemExt)
			itemExt.SetParachuteUnused_Server();

		ParachuteItemComponent item = ResolveParachuteItem_Server(pilot);
		if (!item)
			return;

		TryDeployChuteForPilot(pilot, item, oldChute);
	}

	override protected void RefreshParachuteItemFromInventory(IEntity pilotEntity)
	{
		m_ParachuteItem = ParachuteHelperFunctions.FindParachuteItemInInventory(pilotEntity, false);
	}

	override protected ParachuteItemComponent ResolveParachuteItem_Server(IEntity pilotEntity)
	{
		return ParachuteHelperFunctions.FindParachuteItemInInventory(pilotEntity, true);
	}

	override void Rpc_ServerExitParachute(RplId chuteId, float velocityAtExit)
	{
		if (!IsAuthority())
			return;

		if (!m_bParachuteDeployed)
			return;

		if (chuteId != m_DeployedChuteId)
			return;

		ParachuteDeployedEntityExtended chuteExt = ParachuteDeployedEntityExtended.Cast(m_DeployedParachute);
		bool skipDamage = chuteExt && chuteExt.IsDeployInvincibilityActive();

		if (!skipDamage)
			ApplyLandingDamage(velocityAtExit);
		else
		{
			ParachuteItemComponentExtended itemExt = ParachuteItemComponentExtended.Cast(m_ParachuteItem);
			if (itemExt)
				itemExt.SetParachuteUnused_Server();
		}

		if (m_CompartmentAccess)
			m_CompartmentAccess.AskOwnerToGetOutFromVehicle(EGetOutType.TELEPORT, 0, ECloseDoorAfterActions.LEAVE_OPEN, true, true);
		else
			TryEjectOccupantFromChute(m_DeployedParachute);

		IEntity chuteToDelete = m_DeployedParachute;
		ClearParachuteExitState();
		Rpc(RpcDo_OnParachuteCleared);
		ScheduleChuteDeleteWithPolling(chuteToDelete, false);
	}

	override void DeleteParachuteEntity(IEntity parachute)
	{
		if (!ParachuteHelperFunctions.IsEntityValid(parachute))
		{
			if (parachute && m_ChutePendingDelete == parachute)
				m_ChutePendingDelete = null;
			return;
		}
		TryEjectOccupantFromChute(parachute);
		if (GetGame())
			GetGame().GetCallqueue().CallLater(DeleteParachuteEntityImmediate, DELETE_AFTER_EJECT_DELAY_MS, false, parachute);
	}

	void DeleteParachuteEntityImmediate(IEntity parachute)
	{
		if (!ParachuteHelperFunctions.IsEntityValid(parachute))
			return;
		SCR_EntityHelper.DeleteEntityAndChildren(parachute);
		if (m_ChutePendingDelete == parachute)
			m_ChutePendingDelete = null;
	}
}
