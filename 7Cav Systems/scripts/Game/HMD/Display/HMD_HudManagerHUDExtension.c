//------------------------------------------------------------------------------------------------
//! Hooks SCR_HUDManagerComponent to drive HMD pool HUD overlay.
modded class SCR_HUDManagerComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		HMD_LaserDesignatorReadoutHud.EnsureLayouts(this);
	}

	//------------------------------------------------------------------------------------------------
	static void ResetPoolMirrorHandshakeForNewPlaySession()
	{
	}

	//------------------------------------------------------------------------------------------------
	override void OnUpdate(IEntity owner)
	{
		super.OnUpdate(owner);
		const float ts = 0.016;
		HMD_ElementHudBridge.InitOnce();
		HMD_ElementVisualizationInput.RegisterOnce();
		HMD_ElementVisualizationInput.PollLocalVehicleMountTransition();
		HMD_ElementVisualizationInput.PollPoolMirrorSubscription();
		HMD_ElementHudBridge.Update(ts);
		HMD_LaserDesignatorReadoutHud.Tick(this);
	}
}
