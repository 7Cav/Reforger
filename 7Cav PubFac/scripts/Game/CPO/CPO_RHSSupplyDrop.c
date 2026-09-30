//------------------------------------------------------------------------------------------------
// CPO_RHSSupplyDrop
//
// RHS's CISS supply drop crate never falls: RHS_ReplaceDeployableEntityComponent
// kicks the crate in EOnInit, but that init runs before the RigidBody component
// has created the physics body, so the kick is applied to nothing. The crate
// spawns with zero velocity, the landed heuristic (vertical velocity within
// 0.5 of zero) fires on the first fixed frame and the supply box deploys at
// drop altitude, where it hangs until garbage collection.
//
// This modded class re-kicks the crate on the early fixed frames until it
// genuinely falls, then hands off to RHS's own landing logic for good.
//
// Upstream-fix contract: when RHS fixes the spawn, the crate falls from RHS's
// own kick and this override hands off on the first frame without logging.
// The re-kick print is the retirement signal: it must not appear on a clean
// run. If RHS renames the class or changes the EOnPostFixedFrame signature,
// the Workbench build fails loudly naming this file. If RHS drops the
// component from the prefab, delete this file.
//------------------------------------------------------------------------------------------------
modded class RHS_ReplaceDeployableEntityComponent
{
	[Attribute("2.0", UIWidgets.Slider, "Re-kick watch duration (s)", "0 10 0.5", category : "CPO")]
	protected float m_fCPORekickSec;

	[Attribute("5.0", UIWidgets.Slider, "Minimum height above ground for a re-kick (m)", "0 50 1", category : "CPO")]
	protected float m_fCPORekickMinHeight;

	protected float m_fCPORekickTimer;
	protected bool m_bCPOCrateHasFallen;
	protected bool m_bCPORekickLogged;

	//------------------------------------------------------------------------------------------------
	override void EOnPostFixedFrame(IEntity owner, float timeSlice)
	{
		// While the stall watch is managing the fall, RHS's own fixed-frame
		// logic does not run, so its zero-velocity landed check cannot misfire
		// and schedule the early deploy. Once the crate genuinely falls (or the
		// watch gives up), the loop hands back to RHS permanently.
		if (CPOKickCrateIfStalled(owner, timeSlice))
			return;

		super.EOnPostFixedFrame(owner, timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	// Returns true while the watch is managing the fall, so the caller skips
	// super: RHS's stall check then cannot fire during the kick window at all.
	protected bool CPOKickCrateIfStalled(IEntity owner, float timeSlice)
	{
		if (m_bIsDeployed || m_bCPOCrateHasFallen)
			return false;

		Physics physics = owner.GetPhysics();
		if (!physics)
			return false;

		vector position = owner.GetOrigin();
		vector velocity = physics.GetVelocity();

		if (velocity[1] < -0.6)
		{
			// Falling under RHS's own power: hand off and never interfere again.
			m_bCPOCrateHasFallen = true;
			return false;
		}

		if (position[1] - owner.GetWorld().GetSurfaceY(position[0], position[2]) < m_fCPORekickMinHeight)
			return false;

		if (m_fCPORekickTimer >= m_fCPORekickSec)
			return false;

		m_fCPORekickTimer += timeSlice;

		physics.SetActive(ActiveState.ACTIVE);
		physics.SetVelocity({velocity[0], -5.0, velocity[2]});

		if (!m_bCPORekickLogged)
		{
			m_bCPORekickLogged = true;
			Print("[" + CPO_FactionsInfo.NAME + "] v" + CPO_FactionsInfo.VERSION + " re-kicking a stalled CISS supply drop crate", LogLevel.WARNING);
		}

		return true;
	}
}
