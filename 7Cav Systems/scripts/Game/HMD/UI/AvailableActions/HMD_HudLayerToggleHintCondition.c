//------------------------------------------------------------------------------------------------
//! Control-hints: IFF / designation HUD toggles (**numpad** binds) from vehicle seats allowed by `HMD_ElementVisualizationVehicleComponent`; hidden while **ADS through an HMD handheld designator** (only **T** / **`[` / `]`** there).
//! Match LaserFixesAgain-style conditions: do not gate on `data.IsValid()`; use local pawn resolution like `HMD_VehicleHudLaserSeatCondition` (player controller), not only `data.GetCharacter()`.
[BaseContainerProps()]
class HMD_HudLayerToggleHintCondition : SCR_AvailableActionCondition
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
			return GetReturnResult(false);
		return GetReturnResult(HMD_EntityHmdHelpers.IsLocalEligibleForHmdHudToggleHints());
	}
}
