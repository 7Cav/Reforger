//------------------------------------------------------------------------------------------------
//! Gate: local player occupies a turret compartment whose controller owner matches this entity (or is this entity).
class HMD_ElementVisualizationTurretComponentClass : HMD_ElementVisualizationBaseComponentClass
{
}

class HMD_ElementVisualizationTurretComponent : HMD_ElementVisualizationBaseComponent
{
	//------------------------------------------------------------------------------------------------
	override bool HmdIsVisualizationGateActive()
	{
		IEntity controlled = SCR_PlayerController.GetLocalMainEntity();
		if (!controlled)
			controlled = SCR_PlayerController.GetLocalControlledEntity();
		if (!controlled)
			return false;
		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(controlled.FindComponent(SCR_CompartmentAccessComponent));
		if (!access)
			return false;
		BaseCompartmentSlot slot = access.GetCompartment();
		if (!slot)
			return false;
		if (slot.GetType() != ECompartmentType.TURRET)
			return false;
		BaseControllerComponent slotCtrl = slot.GetController();
		if (!slotCtrl)
			return false;
		IEntity playerTurretEnt = slotCtrl.GetOwner();
		IEntity mine = GetOwner();
		if (!mine || !playerTurretEnt)
			return false;
		if (playerTurretEnt == mine)
			return true;
		IEntity walk = mine;
		while (walk)
		{
			if (walk == playerTurretEnt)
				return true;
			walk = walk.GetParent();
		}
		return false;
	}
}
