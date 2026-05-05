//------------------------------------------------------------------------------------------------
//! Thin `WCS_Armament_HandheldLaserDesignatorComponent` on `Laser_Designation.et`: keeps WCS designation state aligned with the
//! spawned entity world origin so WCS seekers treat this prefab as a valid laser track (no handheld gadget / character trace).
class HMD_LaserDesignationWcsBridgeComponentClass : WCS_Armament_HandheldLaserDesignatorComponentClass
{
}

class HMD_LaserDesignationWcsBridgeComponent : WCS_Armament_HandheldLaserDesignatorComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (Replication.IsServer())
			SetEventMask(owner, EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		if (!Replication.IsServer() || !owner)
			return;
		SetDesignatingState(true);
		SetDesignationPosition(owner.GetOrigin());
	}
}
