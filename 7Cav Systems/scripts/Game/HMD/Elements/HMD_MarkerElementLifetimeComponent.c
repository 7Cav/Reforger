//------------------------------------------------------------------------------------------------
//! Like `HMD_MarkerElementBaseComponent`, but deletes the owner entity after `m_fLifetimeSeconds` on the authority server.
//! Use `0` lifetime to disable auto-delete (default).
[BaseContainerProps()]
class HMD_MarkerElementLifetimeComponentClass : HMD_MarkerElementBaseComponentClass
{
}

class HMD_MarkerElementLifetimeComponent : HMD_MarkerElementBaseComponent
{
	[Attribute("0", UIWidgets.EditBox, "Seconds until this entity is deleted (server only). 0 = no auto-delete.", category: "HMD Lifetime")]
	protected float m_fLifetimeSeconds;

	//! Same Workbench pattern as LaserFixesAgain `HMD_PlacedDesignationComponent` (`UIWidgets.ColorPicker` + `ref Color` + `PackToInt()`).
	[Attribute("1 1 1 1", UIWidgets.ColorPicker, "Pool HUD dot + label color (RGBA 0-1)", category: "HMD Pool HUD")]
	protected ref Color m_cHmdPoolHudColor;

	protected bool m_bLifetimeDeleteScheduled;

	//------------------------------------------------------------------------------------------------
	int HmdGetLifetimePoolHudColorArgb()
	{
		if (m_cHmdPoolHudColor)
			return m_cHmdPoolHudColor.PackToInt();
		return Color.FromRGBA(255, 255, 255, 255).PackToInt();
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		m_bLifetimeDeleteScheduled = false;
		super.OnPostInit(owner);
		HmdScheduleLifetimeDeleteIfNeeded();
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdScheduleLifetimeDeleteIfNeeded()
	{
		if (m_bLifetimeDeleteScheduled)
			return;
		if (!Replication.IsServer())
			return;
		if (m_fLifetimeSeconds <= 0)
			return;
		if (!GetGame())
			return;
		m_bLifetimeDeleteScheduled = true;
		int delayMs = (int)(m_fLifetimeSeconds * 1000.0);
		if (delayMs < 1)
			delayMs = 1;
		GetGame().GetCallqueue().CallLater(HmdLifetimeExpireDeleteOwner, delayMs, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdLifetimeExpireDeleteOwner()
	{
		if (!Replication.IsServer())
			return;
		IEntity ent = GetOwner();
		if (!ent)
			return;
		SCR_EntityHelper.DeleteEntityAndChildren(ent);
	}
}
