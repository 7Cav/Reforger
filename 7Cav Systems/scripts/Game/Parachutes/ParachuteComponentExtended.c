class ParachuteComponentExtendedClass : ParachuteComponentClass {}
class ParachuteComponentExtended : ParachuteComponent
{
	protected static const int PARACHUTE_DELETE_MAX_RETRIES = 20;
	protected static const int PARACHUTE_DELETE_POLL_INTERVAL_MS = 200;
	protected static const int DELETE_AFTER_EJECT_DELAY_MS = 200;
	protected static const int CHUTE_DELETE_DELAY_MS = 200;

	protected bool m_bPilotDeployInvincibilityActive;
	protected IEntity m_ChutePendingDelete;

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
		if (!m_bPilotDeployInvincibilityActive)
			return;

		m_bPilotDeployInvincibilityActive = false;

		if (pilot)
			SetDeployInvincibility(pilot, false);

		ParachuteDeployedEntityExtended chuteExt = ParachuteDeployedEntityExtended.Cast(m_DeployedParachute);
		if (chuteExt)
			chuteExt.EndDeployInvincibilityWhenPilotSeated();
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
			DeleteParachuteEntity(chute);
			if (clearState)
				ClearParachuteExitState_Authority();
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
			GetGame().GetCallqueue().CallLater(DeleteParachuteEntity, CHUTE_DELETE_DELAY_MS, false, chute);
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

	void DeleteParachuteEntityImmediate(IEntity parachute)
	{
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

	override void OnJumpPressed()
	{
		IEntity pilot = GetPilotEntity();
		if (!pilot)
			return;

		if (!m_ParachuteItem)
			return;

		if (!MayDeployParachute_Internal(pilot, m_ParachuteItem))
			return;

		if (IsAuthority())
			RpcAskDeployParachute();
		else
			Rpc(RpcAskDeployParachute);

		BeginDeployInvincibilityUntilSeated();
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

		EnableDeployInvincibility(pilot);

		ResourceName prefab = item.GetParachutePrefab();
		if (prefab == "")
		{
			RestoreDeployInvincibility(pilot);
			return;
		}

		EntitySpawnParams sp = new EntitySpawnParams;
		sp.TransformMode = ETransformMode.WORLD;
		pilot.GetWorldTransform(sp.Transform);

		IEntity spawned = GetGame().SpawnEntityPrefabEx(prefab, false, GetGame().GetWorld(), sp);
		ParachuteDeployedEntity chute = ParachuteDeployedEntity.Cast(spawned);
		if (!chute)
		{
			RestoreDeployInvincibility(pilot);
			return;
		}

		item.SetParachuteUsed_Server();

		m_DeployedParachute = chute;
		m_ParachuteItem = item;

		GiveChuteOwnershipToController(chute);

		BaseCompartmentManagerComponent bcm = BaseCompartmentManagerComponent.Cast(
			chute.FindComponent(BaseCompartmentManagerComponent));
		if (!bcm)
		{
			RestoreDeployInvincibility(pilot);
			return;
		}

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
		{
			RestoreDeployInvincibility(pilot);
			return;
		}

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
	}

	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		if (!SCR_ChimeraCharacter.Cast(to))
			RestoreDeployInvincibility(from);

		super.OnControlledEntityChanged(from, to);
	}

	override void OnRep_DeployState()
	{
		if (IsPilotOwnedLocally() && m_bParachuteDeployed)
			BeginDeployInvincibilityUntilSeated();

		super.OnRep_DeployState();
	}

	override protected void TryEnterChute_Owner()
	{
		if (!GetGame().InPlayMode())
			return;

		IEntity pilot = GetPilotEntity();
		IEntity chute = m_DeployedParachute;
		if (pilot && chute && IronHorseParachuteFixes_Helper.IsPilotSeatedInChute(pilot, chute))
		{
			RestoreDeployInvincibilityIfSeated(pilot, chute);
			WaitForChuteOwnershipThenEnableControls_Owner();
			return;
		}

		super.TryEnterChute_Owner();

		pilot = GetPilotEntity();
		chute = m_DeployedParachute;
		if (pilot && chute)
			RestoreDeployInvincibilityIfSeated(pilot, chute);
	}

	override protected void WaitForChuteOwnershipThenEnableControls_Owner()
	{
		IEntity pilot = GetPilotEntity();
		if (pilot && m_DeployedParachute)
			RestoreDeployInvincibilityIfSeated(pilot, m_DeployedParachute);

		super.WaitForChuteOwnershipThenEnableControls_Owner();
	}

	override protected void OnDestroyed(Instigator killer, IEntity killerEntity)
	{
		if (!IsAuthority())
			return;

		RestoreDeployInvincibility(GetPilotEntity());

		if (m_DeployedParachute)
			ScheduleChuteDeleteWithPolling(m_DeployedParachute, true);
		else
			ClearParachuteExitState_Authority();
	}

	override void RpcDo_OnParachuteCleared()
	{
		RestoreDeployInvincibility(GetPilotEntity());
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

		if (velocityAtExit >= m_fHardLandingVelocity && velocityAtExit < m_fDeathLandingVelocity)
			BreakLegs_Server();
		else if (velocityAtExit >= m_fDeathLandingVelocity)
			KillPlayer_Server();

		IEntity pilot = GetPilotEntity();
		if (pilot)
		{
			RestoreDeployInvincibility(pilot);

			if (!m_CompartmentAccess)
				m_CompartmentAccess = SCR_CompartmentAccessComponent.Cast(
					pilot.FindComponent(SCR_CompartmentAccessComponent));

			if (m_CompartmentAccess)
				m_CompartmentAccess.AskOwnerToGetOutFromVehicle(
					EGetOutType.TELEPORT,
					0,
					ECloseDoorAfterActions.LEAVE_OPEN,
					true,
					true);
		}
		else
		{
			TryDetachPilotFromChute(m_DeployedParachute);
		}

		IEntity chuteToDelete = m_DeployedParachute;
		ClearParachuteExitState_Authority();
		Rpc(RpcDo_OnParachuteCleared);
		ScheduleChuteDeleteWithPolling(chuteToDelete, false);
	}

	override void DeleteParachuteEntity(IEntity parachute)
	{
		if (!IronHorseParachuteFixes_Helper.IsEntityValid(parachute))
		{
			if (parachute && m_ChutePendingDelete == parachute)
				m_ChutePendingDelete = null;
			return;
		}

		TryDetachPilotFromChute(parachute);

		if (GetGame())
			GetGame().GetCallqueue().CallLater(
				DeleteParachuteEntityImmediate,
				DELETE_AFTER_EJECT_DELAY_MS,
				false,
				parachute);
	}
}
