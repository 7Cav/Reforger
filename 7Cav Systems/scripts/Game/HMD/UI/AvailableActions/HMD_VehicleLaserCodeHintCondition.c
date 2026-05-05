//------------------------------------------------------------------------------------------------

//! Control-hints: vehicle numpad laser code steps while the local pawn is mounted (vehicle / crew hierarchy).

//! Match LaserFixesAgain-style conditions: do not gate on `data.IsValid()`; mounted check uses the same local hierarchy as other HMD helpers.

[BaseContainerProps()]

class HMD_VehicleLaserCodeHintCondition : SCR_AvailableActionCondition

{

	//------------------------------------------------------------------------------------------------

	override bool IsEnabled()

	{

		return true;

	}



	//------------------------------------------------------------------------------------------------

	override bool IsAvailable(SCR_AvailableActionsConditionData data)

	{

		if (!data)

		{

			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_VEHICLE_LASER_CODE, false, "no condition data");

			return GetReturnResult(false);

		}

		if (HMD_EntityHmdHelpers.IsLocalHeldHmdLaserDesignatorUsingAds())

		{

			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_VEHICLE_LASER_CODE, false, "handheld designator ADS / viewport (vehicle code binds hidden)");

			return GetReturnResult(false);

		}

		if (!HMD_EntityHmdHelpers.IsLocalMountedInVehicle())

		{

			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_VEHICLE_LASER_CODE, false, "not mounted in vehicle");

			return GetReturnResult(false);

		}

		if (!HMD_EntityHmdHelpers.LocalVehicleRootHasLaserMarkingCapability(12))

		{

			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_VEHICLE_LASER_CODE, false, "vehicle root has no HMD laser marking component");

			return GetReturnResult(false);

		}

		IEntity chEnt = SCR_PlayerController.GetLocalMainEntity();

		if (!chEnt)

			chEnt = SCR_PlayerController.GetLocalControlledEntity();

		if (!chEnt)

		{

			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_VEHICLE_LASER_CODE, false, "no local pawn");

			return GetReturnResult(false);

		}

		IEntity root = chEnt.GetRootParent();

		if (!root || root == chEnt)

		{

			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_VEHICLE_LASER_CODE, false, "invalid vehicle root");

			return GetReturnResult(false);

		}

		HMD_LaserMarkingCoreComponent lm = HMD_EntityHmdHelpers.FindFirstLaserMarkingInHierarchy(root, 12);

		if (!lm || !lm.HmdIsLocalPlayerEligibleForLaserControl())

		{

			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_VEHICLE_LASER_CODE, false, "no laser marking on root or local seat / helmet not eligible for laser control");

			return GetReturnResult(false);

		}

		HMD_LaserMarkingRemoteVehicleComponent rmRef = HMD_LaserMarkingRemoteVehicleComponent.HmdTryResolveLocalEligibleRemoteForControlHints(root);

		if (rmRef && rmRef.HmdIsLaserReferenceMode())

		{

			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_VEHICLE_LASER_CODE, false, "laser reference mode: numpad 8=WP, 7=delete");

			return GetReturnResult(false);

		}

		HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_VEHICLE_LASER_CODE, true, "vehicle laser code binds available");

		return GetReturnResult(true);

	}

}

