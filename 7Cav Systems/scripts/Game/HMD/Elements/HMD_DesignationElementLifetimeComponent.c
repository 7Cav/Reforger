//------------------------------------------------------------------------------------------------
//! Like `HMD_DesignationElementComponent`, but deletes the owner entity after `m_fLifetimeSeconds` on the authority server.
//! Use `0` lifetime to disable auto-delete (default). Prefer short lifetimes on world-placed prefabs, not on player-held gadgets.
[BaseContainerProps()]
class HMD_DesignationElementLifetimeComponentClass : HMD_DesignationElementComponentClass
{
}

class HMD_DesignationElementLifetimeComponent : HMD_DesignationElementComponent
{
	[Attribute("0", UIWidgets.EditBox, "Seconds until this entity is deleted (server only). 0 = no auto-delete.", category: "HMD Lifetime")]
	protected float m_fLifetimeSeconds;

	protected bool m_bLifetimeDeleteScheduled;

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
