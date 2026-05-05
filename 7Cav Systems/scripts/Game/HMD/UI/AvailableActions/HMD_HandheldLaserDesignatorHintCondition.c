//------------------------------------------------------------------------------------------------
//! Control-hints: show handheld laser designator binds only while the designator viewport is active (LaserFixesAgain `HMD_IsBinocularGadgetHeldCondition` / `IsZoomedForHMD`).
//! Match LaserFixesAgain-style conditions: do not gate on `data.IsValid()` (vanilla may not mark the cache valid while binds still work).
[BaseContainerProps()]
class HMD_HandheldLaserDesignatorHintCondition : SCR_AvailableActionCondition
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
			HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_HANDHELD_DESIGNATOR, false, "no condition data");
			return GetReturnResult(false);
		}
		bool ads = HMD_EntityHmdHelpers.IsLocalHeldHmdLaserDesignatorUsingAds();
		string detail = "not in designator viewport";
		if (ads)
			detail = "looking through handheld designator (T / code binds)";
		HMD_HmdDebug.CliEligibilityHintChanged(HMD_HmdDebug.HINT_HANDHELD_DESIGNATOR, ads, detail);
		return GetReturnResult(ads);
	}
}
