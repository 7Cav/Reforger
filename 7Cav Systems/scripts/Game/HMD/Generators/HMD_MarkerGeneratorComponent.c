//------------------------------------------------------------------------------------------------
//! Optional sibling on the same entity as HMD_MarkerElementBaseComponent: pushes attribute defaults into that component during OnPostInit.
class HMD_MarkerGeneratorComponentClass : ScriptComponentClass
{
}

class HMD_MarkerGeneratorComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.EditBox, "Informational class: 0 IFF, 1 Waypoint, 2 Reference, 3 Generic", category: "HMD Generator")]
	protected int m_iInformationalClass;

	[Attribute("MARK", UIWidgets.EditBox, "Pool row label", category: "HMD Generator")]
	protected string m_sPoolLabel;

	//------------------------------------------------------------------------------------------------
	void PushToMarker(HMD_MarkerElementBaseComponent marker)
	{
		if (!marker)
			return;
		marker.HmdApplyGeneratorOverrides(m_iInformationalClass, m_sPoolLabel);
	}
}
