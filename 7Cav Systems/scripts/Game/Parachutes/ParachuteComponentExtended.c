class ParachuteComponentExtendedClass : ParachuteComponentClass {}
class ParachuteComponentExtended : ParachuteComponent
{
	protected static const int PARACHUTE_DELETE_MAX_RETRIES = 20;
	protected static const int PARACHUTE_DELETE_POLL_INTERVAL_MS = 200;
	protected static const int DELETE_AFTER_EJECT_DELAY_MS = 200;
	protected static const int CHUTE_DELETE_DELAY_MS = 200;
	protected static const int SEATING_CHECK_DELAY_MS = 1000;
	protected static const int MAX_SEATING_DEPLOY_ATTEMPTS = 2;

	protected bool m_bPilotDeployInvincibilityActive;
	protected IEntity m_ChutePendingDelete;

	protected bool m_bSeatingWatchActive;
	protected int m_iSeatingDeployAttempts;
	protected IEntity m_SeatingWatchPilot;
	protected ParachuteItemComponent m_SeatingWatchItem;
	protected ResourceName m_SeatingWatchPrefab;

	protected bool IsPilotOwnedLocally()
	{
		IEntity pilot = GetPilotEntity();
		if (!pilot)
			return false;

		RplComponent pilotRpl = RplComponent.Cast(pilot.FindComponent(RplComponent));
		return pilotRpl && pilotRpl.IsOwner();
	}

	protected void SetDeployInvincibility(IEntity pilot, bool invincible)
	{
		if (!pilot)
			return;

		IronHorseParachuteFixes_Helper.SetEntityDamageHandling(pilot, !invincible);
	}

	protected void BeginDeployInvincibilityUntilSeated()
	{
		if (m_bPilotDeployInvincibilityActive)
			return;

		if (!IsPilotOwnedLocally() && !IsAuthority())
			return;

		IEntity pilot = GetPilotEntity();
		if (!pilot)
			return;

		m_bPilotDeployInvincibilityActive = true;
		SetDeployInvincibility(pilot, true);
	}

	protected void RestoreDeployInvincibilityIfSeated(IEntity pilot, IEntity chute)
	{
		if (!m_bPilotDeployInvincibilityActive)
			return;

		if (!pilot || !chute)
			return;

		if (!IronHorseParachuteFixes_Helper.IsPilotSeatedInChute(pilot, chute))
			return;

		StopSeatingWatch_Authority();
		RestoreDeployInvincibility(pilot);
	}

	protected void EnableDeployInvincibility(IEntity pilot)
	{
		if (m_bPilotDeployInvincibilityActive)
			return;

		m_bPilotDeployInvincibilityActive = true;
		SetDeployInvincibility(pilot, true);
	}

	protected void RestoreDeployInvincibility(IEntity pilot)
	{
		GuaranteeRestoreDeployInvincibility(pilot, m_DeployedParachute);
	}

	protected void GuaranteeRestoreDeployInvincibility(IEntity pilot, IEntity chute = null)
	{
		m_bPilotDeployInvincibilityActive = false;

		if (pilot)
			SetDeployInvincibility(pilot, false);

		if (!chute)
			chute = m_DeployedParachute;

		ParachuteDeployedEntityExtended chuteExt = ParachuteDeployedEntityExtended.Cast(chute);
		if (chuteExt)
			chuteExt.ForceEndDeployInvincibility();
	}

	protected void StopSeatingWatch_Authority()
	{
		m_bSeatingWatchActive = false;
		m_iSeatingDeployAttempts = 0;
		m_SeatingWatchPilot = null;
		m_SeatingWatchItem = null;
		m_SeatingWatchPrefab = "";
	}

	protected void StartSeatingWatch_Authority(IEntity pilot, ParachuteItemComponent item, ResourceName prefab)
	{
		if (!IsAuthority() || !GetGame())
			return;

		m_bSeatingWatchActive = true;
		m_SeatingWatchPilot = pilot;
		m_SeatingWatchItem = item;
		m_SeatingWatchPrefab = prefab;

		GetGame().GetCallqueue().CallLater(CheckSeatingWatch_Authority, SEATING_CHECK_DELAY_MS, false);
	}

	protected void CheckSeatingWatch_Authority()
	{
		if (!m_bSeatingWatchActive || !IsAuthority() || !GetGame())
			return;

		IEntity pilot = m_SeatingWatchPilot;
		if (!pilot || !IronHorseParachuteFixes_Helper.IsEntityValid(pilot) || !m_bParachuteDeployed)
		{
			AbortDeploySeating_Authority();
			return;
		}

		IEntity chute = m_DeployedParachute;
		if (chute && IronHorseParachuteFixes_Helper.IsPilotSeatedInChute(pilot, chute))
		{
			StopSeatingWatch_Authority();
			RestoreDeployInvincibility(pilot);
			return;
		}

		if (m_iSeatingDeployAttempts < MAX_SEATING_DEPLOY_ATTEMPTS)
		{
			ReplaceChuteAndRetrySeat_Authority(pilot, m_SeatingWatchItem, m_SeatingWatchPrefab);
			return;
		}

		AbortDeploySeating_Authority();
	}

	protected void ReplaceChuteAndRetrySeat_Authority(IEntity pilot, ParachuteItemComponent item, ResourceName prefab)
	{
		if (!IsAuthority() || !GetGame() || !pilot || !item || prefab == "")
		{
			AbortDeploySeating_Authority();
			return;
		}

		IEntity oldChute = m_DeployedParachute;
		if (oldChute)
		{
			TryDetachPilotFromChute(oldChute);
			DeleteParachuteEntityImmediate(oldChute, false);
		}

		ParachuteDeployedEntity chute = SpawnChuteAtPilot_Authority(pilot, prefab);
		if (!chute)
		{
			AbortDeploySeating_Authority();
			return;
		}

		if (!FinishChuteSetup_Authority(pilot, item, chute))
		{
			DeleteParachuteEntityImmediate(chute, false);
			AbortDeploySeating_Authority();
			return;
		}

		m_iSeatingDeployAttempts++;
		GetGame().GetCallqueue().CallLater(CheckSeatingWatch_Authority, SEATING_CHECK_DELAY_MS, false);
	}

	protected ParachuteDeployedEntity SpawnChuteAtPilot_Authority(IEntity pilot, ResourceName prefab)
	{
		EntitySpawnParams sp = new EntitySpawnParams;
		sp.TransformMode = ETransformMode.WORLD;
		pilot.GetWorldTransform(sp.Transform);

		IEntity spawned = GetGame().SpawnEntityPrefabEx(prefab, false, GetGame().GetWorld(), sp);
		return ParachuteDeployedEntity.Cast(spawned);
	}

	protected bool FinishChuteSetup_Authority(IEntity pilot, ParachuteItemComponent item, ParachuteDeployedEntity chute)
	{
		if (!chute || !pilot || !item)
			return false;

		m_DeployedParachute = chute;
		m_ParachuteItem = item;

		GiveChuteOwnershipToController(chute);

		BaseCompartmentManagerComponent bcm = BaseCompartmentManagerComponent.Cast(
			chute.FindComponent(BaseCompartmentManagerComponent));
		if (!bcm)
			return false;

		array<BaseCompartmentSlot> slots = {};
		bcm.GetCompartments(slots);

		BaseCompartmentSlot pilotSlot = null;
		foreach (BaseCompartmentSlot s : slots)
		{
			if (!s)
				continue;
			if (s.GetType() == ECompartmentType.CARGO)
			{
				pilotSlot = s;
				break;
			}
		}

		if (!pilotSlot)
			return false;

		m_vDeployVelocity = pilot.GetPhysics().GetVelocity();

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(
			pilot.FindComponent(SCR_CompartmentAccessComponent));
		chute.InitializePilot(pilot, access, m_vDeployVelocity);

		ParachuteDeployedEntityExtended chuteExt = ParachuteDeployedEntityExtended.Cast(chute);
		if (chuteExt)
			chuteExt.StartDeployInvincibilityUntilSeated();

		m_DeployedChuteId = chute.GetRplId();
		m_iChuteSlotId = pilotSlot.GetCompartmentSlotID();
		m_bParachuteDeployed = true;

		Replication.BumpMe();
		GetGame().GetCallqueue().CallLater(
			Do_SetupDeployedChute_Owner,
			50,
			false,
			m_DeployedChuteId,
			m_iChuteSlotId,
			m_vDeployVelocity);

		return true;
	}

	protected void AbortDeploySeating_Authority()
	{
		if (!IsAuthority())
			return;

		IEntity pilot = m_SeatingWatchPilot;
		if (!pilot)
			pilot = GetPilotEntity();

		StopSeatingWatch_Authority();

		IEntity chute = m_DeployedParachute;
		GuaranteeRestoreDeployInvincibility(pilot, chute);

		if (chute)
		{
			TryDetachPilotFromChute(chute);
			DeleteParachuteEntityImmediate(chute, false);
		}

		ClearParachuteExitState_Authority();
		Rpc(RpcDo_OnParachuteCleared);
	}

	protected void TryDetachPilotFromChute(IEntity chute)
	{
		IronHorseParachuteFixes_Helper.EjectOccupantFromSlot(
			IronHorseParachuteFixes_Helper.FindCargoSlotOnEntity(chute));
	}

	protected void ClearParachuteExitState_Authority()
	{
		if (!IsAuthority())
			return;

		m_DeployedParachute = null;
		m_bParachuteDeployed = false;
		m_DeployedChuteId = RplId.Invalid();
		m_iChuteSlotId = -1;
		Replication.BumpMe();
	}

	void PollUntilEmptyThenDeleteChute(IEntity chute, int retryCount, bool clearState = true)
	{
		if (!chute)
		{
			if (clearState)
				ClearParachuteExitState_Authority();
			return;
		}

		if (!IronHorseParachuteFixes_Helper.IsEntityValid(chute))
		{
			if (m_ChutePendingDelete == chute)
				m_ChutePendingDelete = null;
			if (clearState)
				ClearParachuteExitState_Authority();
			return;
		}

		if (retryCount >= PARACHUTE_DELETE_MAX_RETRIES)
		{
			DeleteParachuteEntityInternal(chute, clearState);
			return;
		}

		if (!GetGame())
		{
			m_ChutePendingDelete = null;
			if (clearState)
				ClearParachuteExitState_Authority();
			return;
		}

		if (IronHorseParachuteFixes_Helper.IsChuteCompartmentEmpty(chute))
		{
			GetGame().GetCallqueue().CallLater(DeleteParachuteEntityInternal, CHUTE_DELETE_DELAY_MS, false, chute, clearState);
			if (clearState)
				ClearParachuteExitState_Authority();
			return;
		}

		GetGame().GetCallqueue().CallLater(
			PollUntilEmptyThenDeleteChute,
			PARACHUTE_DELETE_POLL_INTERVAL_MS,
			true,
			chute,
			retryCount + 1,
			clearState);
	}

	void ScheduleChuteDeleteWithPolling(IEntity chute, bool clearState = true)
	{
		if (!IronHorseParachuteFixes_Helper.IsEntityValid(chute))
			return;

		if (chute == m_ChutePendingDelete)
			return;

		m_ChutePendingDelete = chute;
		TryDetachPilotFromChute(chute);

		if (clearState)
			ClearParachuteExitState_Authority();

		if (GetGame())
			GetGame().GetCallqueue().CallLater(
				PollUntilEmptyThenDeleteChute,
				PARACHUTE_DELETE_POLL_INTERVAL_MS,
				true,
				chute,
				0,
				clearState);
	}

	void DeleteParachuteEntityImmediate(IEntity parachute, bool restoreInvincibility = true)
	{
		if (restoreInvincibility)
			GuaranteeRestoreDeployInvincibility(GetPilotEntity(), parachute);

		if (!IronHorseParachuteFixes_Helper.IsEntityValid(parachute))
		{
			if (parachute && m_ChutePendingDelete == parachute)
				m_ChutePendingDelete = null;
			return;
		}

		SCR_EntityHelper.DeleteEntityAndChildren(parachute);

		if (m_ChutePendingDelete == parachute)
			m_ChutePendingDelete = null;
	}

	override void RpcAskDeployParachute()
	{
		if (m_bParachuteDeployed || m_bSeatingWatchActive)
			return;

		IEntity pilot = GetPilotEntity();
		if (!pilot)
			return;

		ParachuteItemComponent item = ResolveParachuteItem_Server(pilot);
		if (!item)
			return;

		if (!MayDeployParachute_Internal(pilot, item))
			return;

		EnableDeployInvincibility(pilot);

		ResourceName prefab = item.GetParachutePrefab();
		if (prefab == "")
		{
			GuaranteeRestoreDeployInvincibility(pilot);
			return;
		}

		ParachuteDeployedEntity chute = SpawnChuteAtPilot_Authority(pilot, prefab);
		if (!chute)
		{
			GuaranteeRestoreDeployInvincibility(pilot);
			return;
		}

		item.SetParachuteUsed_Server();

		if (!FinishChuteSetup_Authority(pilot, item, chute))
		{
			DeleteParachuteEntityImmediate(chute, true);
			GuaranteeRestoreDeployInvincibility(pilot);
			return;
		}

		m_iSeatingDeployAttempts = 1;
		StartSeatingWatch_Authority(pilot, item, prefab);
	}

	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		if (!SCR_ChimeraCharacter.Cast(to))
		{
			StopSeatingWatch_Authority();
			GuaranteeRestoreDeployInvincibility(from);
		}

		super.OnControlledEntityChanged(from, to);
	}

	override void RpcDo_OnParachuteCleared()
	{
		StopSeatingWatch_Authority();
		GuaranteeRestoreDeployInvincibility(GetPilotEntity());
		super.RpcDo_OnParachuteCleared();
	}

	override void Rpc_ServerExitParachute(RplId chuteId, float velocityAtExit)
	{
		if (!IsAuthority())
			return;

		if (!m_bParachuteDeployed)
			return;

		if (chuteId != m_DeployedChuteId)
			return;

		StopSeatingWatch_Authority();

		if (velocityAtExit >= m_fHardLandingVelocity && velocityAtExit < m_fDeathLandingVelocity)
			BreakLegs_Server();
		else if (velocityAtExit >= m_fDeathLandingVelocity)
			KillPlayer_Server();

		IEntity pilot = GetPilotEntity();
		IEntity chuteToDelete = m_DeployedParachute;
		GuaranteeRestoreDeployInvincibility(pilot, chuteToDelete);

		if (pilot)
		{
			if (!m_CompartmentAccess)
				m_CompartmentAccess = SCR_CompartmentAccessComponent.Cast(
					pilot.FindComponent(SCR_CompartmentAccessComponent));

			if (m_CompartmentAccess)
				m_CompartmentAccess.AskOwnerToGetOutFromVehicle(
					EGetOutType.TELEPORT, 0, ECloseDoorAfterActions.LEAVE_OPEN, true, true);
		}
		else
		{
			TryDetachPilotFromChute(chuteToDelete);
		}

		ClearParachuteExitState_Authority();
		Rpc(RpcDo_OnParachuteCleared);
		ScheduleChuteDeleteWithPolling(chuteToDelete, false);
	}

	override void DeleteParachuteEntity(IEntity parachute)
	{
		DeleteParachuteEntityInternal(parachute, true);
	}

	protected void DeleteParachuteEntityInternal(IEntity parachute, bool restoreInvincibility)
	{
		if (!IronHorseParachuteFixes_Helper.IsEntityValid(parachute))
		{
			if (parachute && m_ChutePendingDelete == parachute)
				m_ChutePendingDelete = null;
			if (restoreInvincibility)
				GuaranteeRestoreDeployInvincibility(GetPilotEntity(), parachute);
			return;
		}

		TryDetachPilotFromChute(parachute);

		if (GetGame())
			GetGame().GetCallqueue().CallLater(
				DeleteParachuteEntityDeferred,
				DELETE_AFTER_EJECT_DELAY_MS,
				false,
				parachute,
				restoreInvincibility);
	}

	protected void DeleteParachuteEntityDeferred(IEntity parachute, bool restoreInvincibility)
	{
		DeleteParachuteEntityImmediate(parachute, restoreInvincibility);
	}
}
