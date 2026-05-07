//------------------------------------------------------------------------------------------------
//! Laser marking on a **turret / sight entity** when only the player **physically occupying that turret compartment**
//! (controller owner matches this entity) may toggle marking and change codes. Optional seat tokens further restrict
//! by compartment identity (e.g. gunner vs copilot) after the turret gate passes.
[BaseContainerProps()]
class HMD_LaserMarkingOccupiedTurretComponentClass : HMD_LaserMarkingComponentClass
{
}

class HMD_LaserMarkingOccupiedTurretComponent : HMD_LaserMarkingComponent
{
	//------------------------------------------------------------------------------------------------
	protected static bool HmdLocalOccupantTurretSlotControlsEntity(IEntity controlled, IEntity markingOwner)
	{
		if (!controlled || !markingOwner)
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
		if (!playerTurretEnt)
			return false;
		if (playerTurretEnt == markingOwner)
			return true;
		IEntity walk = markingOwner;
		while (walk)
		{
			if (walk == playerTurretEnt)
				return true;
			walk = walk.GetParent();
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool HmdEvaluateLocalLaserControlEligibility(IEntity controlled)
	{
		if (!HmdControlledSharesMarkingVehicleRoot(controlled))
			return false;
		IEntity markingEnt = GetOwner();
		if (!markingEnt)
			return false;
		if (!HmdLocalOccupantTurretSlotControlsEntity(controlled, markingEnt))
			return false;
		if (!HmdPassesHelmetForLaserControl(controlled))
			return false;
		if (m_aLaserControlSeatTokens.IsEmpty())
			return true;
		return HmdEvaluateLaserControlByConfiguredSeatTokens(controlled);
	}
}
