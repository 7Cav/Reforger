//------------------------------------------------------------------------------------------------
//! Shared preamble + mutually exclusive control-hint rows for **laser reference mode** vs normal
//! remote vehicle laser binds (same `m_sAction` keys, different `m_sName` in `AvailableActions.conf`).
class HMD_LaserReferenceHintEval
{
	//------------------------------------------------------------------------------------------------
	static bool Preamble(SCR_AvailableActionsConditionData data, out HMD_LaserMarkingRemoteVehicleComponent rm)
	{
		rm = null;
		if (!data)
			return false;
		if (HMD_EntityHmdHelpers.IsLocalHeldHmdLaserDesignatorUsingAds())
			return false;
		if (!HMD_EntityHmdHelpers.IsLocalMountedInVehicle())
			return false;
		if (!HMD_EntityHmdHelpers.LocalVehicleRootHasLaserMarkingCapability(12))
			return false;
		IEntity chEnt = SCR_PlayerController.GetLocalMainEntity();
		if (!chEnt)
			chEnt = SCR_PlayerController.GetLocalControlledEntity();
		if (!chEnt)
			return false;
		IEntity root = chEnt.GetRootParent();
		if (!root || root == chEnt)
			return false;
		rm = HMD_LaserMarkingRemoteVehicleComponent.HmdTryResolveLocalEligibleRemoteForControlHints(root);
		return rm != null;
	}
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class HMD_LaserReferenceModeEnterHintCondition : SCR_AvailableActionCondition
{
	override bool IsEnabled()
	{
		return true;
	}

	override bool IsAvailable(SCR_AvailableActionsConditionData data)
	{
		HMD_LaserMarkingRemoteVehicleComponent rm;
		if (!HMD_LaserReferenceHintEval.Preamble(data, rm))
		{
			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_LASER_REFERENCE_ENTER, false, "preamble failed");
			return GetReturnResult(false);
		}
		if (rm.HmdIsLaserReferenceMode())
		{
			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_LASER_REFERENCE_ENTER, false, "already in laser reference mode");
			return GetReturnResult(false);
		}
		HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_LASER_REFERENCE_ENTER, true, "enter laser reference mode available");
		return GetReturnResult(true);
	}
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class HMD_LaserReferenceModeExitHintCondition : SCR_AvailableActionCondition
{
	override bool IsEnabled()
	{
		return true;
	}

	override bool IsAvailable(SCR_AvailableActionsConditionData data)
	{
		HMD_LaserMarkingRemoteVehicleComponent rm;
		if (!HMD_LaserReferenceHintEval.Preamble(data, rm))
		{
			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_LASER_REFERENCE_EXIT, false, "preamble failed");
			return GetReturnResult(false);
		}
		if (!rm.HmdIsLaserReferenceMode())
		{
			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_LASER_REFERENCE_EXIT, false, "not in laser reference mode");
			return GetReturnResult(false);
		}
		HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_LASER_REFERENCE_EXIT, true, "exit laser reference mode available");
		return GetReturnResult(true);
	}
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class HMD_LaserReferencePlaceRpHintCondition : SCR_AvailableActionCondition
{
	override bool IsEnabled()
	{
		return true;
	}

	override bool IsAvailable(SCR_AvailableActionsConditionData data)
	{
		HMD_LaserMarkingRemoteVehicleComponent rm;
		if (!HMD_LaserReferenceHintEval.Preamble(data, rm))
		{
			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_LASER_REFERENCE_PLACE_RP, false, "preamble failed");
			return GetReturnResult(false);
		}
		if (!rm.HmdIsLaserReferenceMode())
		{
			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_LASER_REFERENCE_PLACE_RP, false, "not in laser reference mode");
			return GetReturnResult(false);
		}
		HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_LASER_REFERENCE_PLACE_RP, true, "place RP marker (numpad /)");
		return GetReturnResult(true);
	}
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class HMD_LaserReferencePlaceWpHintCondition : SCR_AvailableActionCondition
{
	override bool IsEnabled()
	{
		return true;
	}

	override bool IsAvailable(SCR_AvailableActionsConditionData data)
	{
		HMD_LaserMarkingRemoteVehicleComponent rm;
		if (!HMD_LaserReferenceHintEval.Preamble(data, rm))
		{
			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_LASER_REFERENCE_PLACE_WP, false, "preamble failed");
			return GetReturnResult(false);
		}
		if (!rm.HmdIsLaserReferenceMode())
		{
			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_LASER_REFERENCE_PLACE_WP, false, "not in laser reference mode");
			return GetReturnResult(false);
		}
		HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_LASER_REFERENCE_PLACE_WP, true, "place WP marker (numpad 8)");
		return GetReturnResult(true);
	}
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class HMD_LaserReferenceDeleteHintCondition : SCR_AvailableActionCondition
{
	override bool IsEnabled()
	{
		return true;
	}

	override bool IsAvailable(SCR_AvailableActionsConditionData data)
	{
		HMD_LaserMarkingRemoteVehicleComponent rm;
		if (!HMD_LaserReferenceHintEval.Preamble(data, rm))
		{
			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_LASER_REFERENCE_DELETE, false, "preamble failed");
			return GetReturnResult(false);
		}
		if (!rm.HmdIsLaserReferenceMode())
		{
			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_LASER_REFERENCE_DELETE, false, "not in laser reference mode");
			return GetReturnResult(false);
		}
		HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_LASER_REFERENCE_DELETE, true, "delete nearest WP/RP (numpad 7, 10m)");
		return GetReturnResult(true);
	}
}
