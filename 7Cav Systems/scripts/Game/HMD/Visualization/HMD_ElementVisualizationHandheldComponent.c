//------------------------------------------------------------------------------------------------
//! Gate: local player is holding this gadget entity as the active gadget (designator raised path handled by WCS).
class HMD_ElementVisualizationHandheldComponentClass : HMD_ElementVisualizationBaseComponentClass
{
}

class HMD_ElementVisualizationHandheldComponent : HMD_ElementVisualizationBaseComponent
{
	//------------------------------------------------------------------------------------------------
	override bool HmdIsVisualizationGateActive()
	{
		return HMD_EntityHmdHelpers.IsLocalLookingThroughLaserDesignatorOnGadgetEntity(GetOwner());
	}
}
