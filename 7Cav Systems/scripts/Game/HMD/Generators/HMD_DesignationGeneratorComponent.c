//------------------------------------------------------------------------------------------------
//! Optional sibling on the same entity as HMD_DesignationElementBaseComponent: pushes attribute defaults into that component during OnPostInit.
class HMD_DesignationGeneratorComponentClass : ScriptComponentClass
{
}

class HMD_DesignationGeneratorComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.EditBox, "Target designation class: 0 Lock, 1 Generic", category: "HMD Generator")]
	protected int m_iDesignationClass;

	[Attribute("LASE", UIWidgets.EditBox, "Pool row label", category: "HMD Generator")]
	protected string m_sPoolLabel;

	[Attribute("1111", UIWidgets.EditBox, "Laser code min (inclusive)", category: "HMD Generator")]
	protected int m_iLaserCodeMin;

	[Attribute("1199", UIWidgets.EditBox, "Laser code max (inclusive)", category: "HMD Generator")]
	protected int m_iLaserCodeMax;

	[Attribute("1111", UIWidgets.EditBox, "Initial laser code (clamped to min/max)", category: "HMD Generator")]
	protected int m_iInitialLaserCode;

	//------------------------------------------------------------------------------------------------
	void PushToDesignation(HMD_DesignationElementBaseComponent des)
	{
		if (!des)
			return;
		des.HmdApplyGeneratorOverrides(m_iDesignationClass, m_sPoolLabel);
		des.HmdApplyLaserCodePolicy(m_iLaserCodeMin, m_iLaserCodeMax, m_iInitialLaserCode);
	}
}
