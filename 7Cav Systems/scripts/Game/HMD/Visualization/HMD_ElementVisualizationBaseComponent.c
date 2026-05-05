//------------------------------------------------------------------------------------------------
//! Base for gated HMD world-overlay contexts (handheld, turret, vehicle hull).
class HMD_ElementVisualizationBaseComponentClass : ScriptComponentClass
{
}

class HMD_ElementVisualizationBaseComponent : ScriptComponent
{
	[Attribute("2000", UIWidgets.EditBox, "Max world distance (m) for HUD dots", category: "HMD")]
	protected float m_fMaxViewDistanceM;

	//------------------------------------------------------------------------------------------------
	float HmdGetMaxViewDistanceM()
	{
		return m_fMaxViewDistanceM;
	}

	//------------------------------------------------------------------------------------------------
	bool HmdIsVisualizationGateActive()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		HMD_ElementVisualizationRegistry.Register(this);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		HMD_ElementVisualizationRegistry.Unregister(this);
		super.OnDelete(owner);
	}
}
