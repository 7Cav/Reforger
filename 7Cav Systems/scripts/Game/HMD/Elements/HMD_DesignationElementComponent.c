//------------------------------------------------------------------------------------------------
//! Target-designation metadata: pool **class** and **label** only; pool registration and laser code live on `HMD_DesignationBaseComponent`.
class HMD_DesignationElementComponentClass : HMD_DesignationBaseComponentClass
{
}

class HMD_DesignationElementComponent : HMD_DesignationBaseComponent
{
	[Attribute("1", UIWidgets.EditBox, "Target designation class: 0 Lock, 1 Generic, 3 IFF-style (pool HUD shows tag only)", category: "HMD")]
	protected int m_iTargetDesignationClass;

	[Attribute("LASE", UIWidgets.EditBox, "Pool row label text", category: "HMD")]
	protected string m_sPoolLabel;

	[Attribute("0", UIWidgets.CheckBox, "When set, pool HUD draws this designation while global designation HUD is hidden, if local vehicle laser marking is ON", category: "HMD")]
	protected bool m_bVehicleLaserOwnHudBypass;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		HmdSetDesignationMetadata(m_iTargetDesignationClass, m_sPoolLabel);
		HmdSetPoolRowVehicleLaserHudBypass(m_bVehicleLaserOwnHudBypass);
		super.OnPostInit(owner);
	}
}
