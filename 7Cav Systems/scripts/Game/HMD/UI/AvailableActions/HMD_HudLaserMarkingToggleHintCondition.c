//------------------------------------------------------------------------------------------------

//! Control-hints: `HMD_HUDLaserMarkingToggle` (**KC_DIVIDE** = numpad `/`). In a vehicle with `HMD_LaserMarkingComponent`, toggles **vehicle laser marking**. Hidden while **ADS through an HMD handheld designator** (use **T** / **`[` / `]`** only).

//! Match LaserFixesAgain-style conditions: do not gate on `data.IsValid()`; use local pawn resolution.

[BaseContainerProps()]

class HMD_HudLaserMarkingToggleHintCondition : SCR_AvailableActionCondition

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

			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_HUD_LASER_MARKING_TOGGLE, false, "no condition data");

			return GetReturnResult(false);

		}

		if (HMD_EntityHmdHelpers.IsLocalHeldHmdLaserDesignatorUsingAds())

		{

			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_HUD_LASER_MARKING_TOGGLE, false, "handheld designator ADS / viewport");

			return GetReturnResult(false);

		}

		IEntity chEnt = SCR_PlayerController.GetLocalMainEntity();

		if (!chEnt)

			chEnt = SCR_PlayerController.GetLocalControlledEntity();

		if (!chEnt)

		{

			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_HUD_LASER_MARKING_TOGGLE, false, "no local pawn");

			return GetReturnResult(false);

		}

		if (!HMD_EntityHmdHelpers.LocalVehicleRootHasLaserMarkingCapability(12))

		{

			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_HUD_LASER_MARKING_TOGGLE, false, "vehicle root has no laser marking");

			return GetReturnResult(false);

		}

		IEntity root = chEnt.GetRootParent();

		if (!root || root == chEnt)

		{

			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_HUD_LASER_MARKING_TOGGLE, false, "invalid vehicle root");

			return GetReturnResult(false);

		}

		HMD_LaserMarkingCoreComponent lm = HMD_EntityHmdHelpers.FindFirstLaserMarkingInHierarchy(root, 12);

		if (!lm || !lm.HmdIsLocalPlayerEligibleForLaserControl())

		{

			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_HUD_LASER_MARKING_TOGGLE, false, "laser marking not found or seat / helmet ineligible");

			return GetReturnResult(false);

		}

		HMD_LaserMarkingRemoteVehicleComponent rmRef = HMD_LaserMarkingRemoteVehicleComponent.HmdTryResolveLocalEligibleRemoteForControlHints(root);

		if (rmRef && rmRef.HmdIsLaserReferenceMode())

		{

			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_HUD_LASER_MARKING_TOGGLE, false, "laser reference mode: numpad / places RP");

			return GetReturnResult(false);

		}

		HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_HUD_LASER_MARKING_TOGGLE, true, "vehicle laser marking toggle hint available");

		return GetReturnResult(true);

	}

}

