class ParachuteDeployedEntityExtendedClass : ParachuteDeployedEntityClass {}
class ParachuteDeployedEntityExtended : ParachuteDeployedEntity
{
	[Attribute("2.0", UIWidgets.Slider, "Empty chute cleanup delay (s)", "1 30 1", category : "Landing")]
	protected float m_fEmptyChuteCleanupDelaySec = 2.0;

	[Attribute("250", UIWidgets.Slider, "Empty chute delete delay (ms)", "50 500 50", category : "Landing")]
	protected int m_iEmptyChuteDeleteDelayMs = 250;

	[Attribute("4.0", UIWidgets.Slider, "Collapse fall speed (m/s)", "1 15 0.5", category : "Landing")]
	protected float m_fCollapseFallSpeedMps = 4.0;

	[Attribute("40", UIWidgets.Slider, "Collapse tilt range (deg)", "0 90 5", category : "Landing")]
	protected float m_fCollapseTiltRangeDeg = 40;

	[Attribute("4.0", UIWidgets.Slider, "Collapse acceleration factor", "1 10 0.5", category : "Landing")]
	protected float m_fCollapseAccelerationFactor = 4.0;

	[Attribute("1.0", UIWidgets.Slider, "Terrain contact threshold (m)", "0.1 3.0 0.1", category : "Landing")]
	protected float m_fTerrainContactThresholdM = 1.0;

	[Attribute("15.0", UIWidgets.Slider, "Max collapse sync duration (s)", "5 60 1", category : "Landing")]
	protected float m_fCollapseSyncMaxDurationSec = 15.0;

	protected bool m_bDeployInvincibilityActive;
	protected PhysicsBlock m_InvincibilityPhysicsBlock;
	// -1 = not yet tracking (compartment was occupied)
	protected float m_fEmptyCompartmentAccumulator = -1;
	protected bool m_bIsCollapsing;
	protected float m_fCollapseTimeAccumulator = 0;

	bool IsDeployInvincibilityActive()
	{
		return m_bDeployInvincibilityActive;
	}

	[Attribute("3.0", UIWidgets.Slider, "Delay before max fall speed applies (s)", "1 15 0.5", category : "Flight")]
	protected float m_fMaxFallSpeedDelaySec = 3.0;

	protected float m_fDeployTimeAccumulator = 0;

	override void InitializePilot(IEntity pilot, SCR_CompartmentAccessComponent access, vector initialVelocity)
	{
		// Set starting vertical (downward) speed = horizontal speed + vertical speed at deploy
		vector horizVel = initialVelocity;
		horizVel[1] = 0;
		float horizLen = horizVel.Length();
		float downwardAtDeploy = Math.Max(0, -initialVelocity[1]);

		vector vel = initialVelocity;
		float targetDownward = horizLen + downwardAtDeploy;
		if (targetDownward < 3.0)
			targetDownward = 3.0;
		vel[1] = -targetDownward;

		m_fDeployTimeAccumulator = 0;

		super.InitializePilot(pilot, access, vel);

		// Base leaves m_fForwardSpeed at 0; HandleGlide then uses it and zeroes horizontal velocity
		// every frame. Initialize from pilot's horizontal speed so we preserve momentum at deploy.
		if (horizLen > 0.01)
			m_fForwardSpeed = Math.Clamp(horizLen, m_MinForwardSpeed, m_MaxForwardSpeed);
	}

	protected void EjectOccupantIfAny()
	{
		ParachuteHelperFunctions.EjectOccupantFromSlot(m_Compartment);
	}

	protected void SendCollapseSyncState()
	{
		GetWorldTransform(m_vWorldTransform);
		vector syncVel = m_Physics.GetVelocity();
		vector syncAngVel = m_Physics.GetAngularVelocity();
		SanitizeNetState(m_vWorldTransform, syncVel, syncAngVel);
		Rpc(RpcDo_SyncMovement, m_vWorldTransform, syncVel, syncAngVel, m_WindDirDeg, m_WindSpeed);
		m_LastPos = m_vWorldTransform[3];
		m_LastAnglesRad = Math3D.MatrixToAngles(m_vWorldTransform) * Math.DEG2RAD;
	}

	protected void StartCollapsePhysics()
	{
		if (!m_Physics)
			return;

		m_Physics.SetInteractionLayer(0);
		m_bIsCollapsing = true;

		float tiltPitch = Math.RandomFloatInclusive(-m_fCollapseTiltRangeDeg, m_fCollapseTiltRangeDeg) * Math.DEG2RAD;
		float tiltRoll = Math.RandomFloatInclusive(-m_fCollapseTiltRangeDeg, m_fCollapseTiltRangeDeg) * Math.DEG2RAD;
		vector angVel = m_Physics.GetAngularVelocity();
		vector pitchAxis = VectorToParent(vector.Right);
		vector rollAxis = VectorToParent(vector.Forward);
		angVel = angVel + (pitchAxis * tiltPitch) + (rollAxis * tiltRoll);
		m_Physics.SetAngularVelocity(angVel);
	}

	void StartDeployInvincibility(float durationSeconds)
	{
		if (durationSeconds <= 0)
			return;

		if (!GetGame())
			return;

		m_bDeployInvincibilityActive = true;

		if (m_DamageManager)
			m_DamageManager.EnableDamageHandling(false);

		// Do not use PhysicsBlock - it constrains chute to pilot physics; when pilot is seated
		// their physics are often disabled, which freezes the chute (especially at high horizontal speed).
		// Damage invincibility is handled by EnableDamageHandling above and pilot damage handling in ParachuteComponentExtended.

		GetGame().GetCallqueue().CallLater(EndDeployInvincibility, (int)(durationSeconds * 1000), false);
	}

	protected void ClearInvincibilityPhysicsBlock()
	{
		if (m_InvincibilityPhysicsBlock)
		{
			m_InvincibilityPhysicsBlock.Remove(this);
			m_InvincibilityPhysicsBlock = null;
		}
	}

	protected void EndDeployInvincibility()
	{
		m_bDeployInvincibilityActive = false;
		ClearInvincibilityPhysicsBlock();

		if (m_DamageManager)
			m_DamageManager.EnableDamageHandling(true);

		RequestRespawnChuteForDisconnectedPilot();
	}

	protected void RequestRespawnChuteForDisconnectedPilot()
	{
		if (!IsAuthority())
			return;

		if (!m_Compartment || !ParachuteHelperFunctions.IsEntityValid(m_Pilot))
			return;

		if (m_Compartment.IsOccupied())
			return;

		if (m_bHasLanded)
			return;

		ParachuteComponentExtended parachuteComp = ParachuteHelperFunctions.GetParachuteComponentFromPilot(m_Pilot);
		if (!parachuteComp)
			return;

		parachuteComp.RespawnChuteForDisconnectedPilot(this, m_Pilot);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		if (!IsAuthority() || !m_Compartment || m_bDeployInvincibilityActive || m_bIsDestroyed)
			return;

		if (m_Compartment.IsOccupied())
		{
			m_fEmptyCompartmentAccumulator = -1;
			return;
		}

		if (m_fEmptyCompartmentAccumulator < 0)
			m_fEmptyCompartmentAccumulator = 0;

		m_fEmptyCompartmentAccumulator = m_fEmptyCompartmentAccumulator + timeSlice;

		if (m_fEmptyCompartmentAccumulator >= m_fEmptyChuteCleanupDelaySec)
			DestroyParachute();
	}

	override void EOnSimulate(IEntity owner, float timeSlice)
	{
		if (!m_RplComponent || !m_Physics)
			return;

		if (!IsAuthority())
			return;

		if (m_bHasLanded && m_Compartment && !m_Compartment.IsOccupied())
		{
			if (!m_bIsCollapsing)
			{
				StartCollapsePhysics();
				m_fCollapseTimeAccumulator = 0;
			}

			m_fCollapseTimeAccumulator += timeSlice;

			vector vel = m_Physics.GetVelocity();
			float downward = -vel[1];
			if (downward < m_fCollapseFallSpeedMps)
			{
				float addDown = (m_fCollapseFallSpeedMps - downward) * timeSlice * m_fCollapseAccelerationFactor;
				vel[1] = vel[1] - addDown;
				m_Physics.SetVelocity(vel);
			}

			if (m_fCollapseTimeAccumulator < m_fCollapseSyncMaxDurationSec)
			{
				m_NetAccTime += timeSlice;
				if (m_NetAccTime >= m_NetSendInterval)
				{
					m_NetAccTime = 0;
					SendCollapseSyncState();
				}
			}
			return;
		}

		m_fDeployTimeAccumulator += timeSlice;
		float origMaxFallSpeed = m_MaxFallSpeed;
		if (m_fDeployTimeAccumulator < m_fMaxFallSpeedDelaySec)
			m_MaxFallSpeed = 9999.0;

		super.EOnSimulate(owner, timeSlice);

		m_MaxFallSpeed = origMaxFallSpeed;
	}

	[Attribute("3000", UIWidgets.Slider, "Fallback collapse delay when exit path fails (ms)", "1000 10000 500", category : "Landing")]
	protected int m_iFallbackCollapseDelayMs = 3000;

	void FallbackEjectAndDelete()
	{
		if (!IsAuthority())
			return;

		ParachuteHelperFunctions.EjectOccupantFromSlot(m_Compartment);

		if (GetGame())
			GetGame().GetCallqueue().CallLater(ParachuteHelperFunctions.DeleteEntityIfValid, m_iFallbackCollapseDelayMs, false, this);
	}

	override void RpcAsk_ServerExitRequest(float velocityAtExit)
	{
		if (!m_Compartment)
		{
			FallbackEjectAndDelete();
			return;
		}

		IEntity occupant = m_Compartment.GetOccupant();
		if (!occupant)
		{
			FallbackEjectAndDelete();
			return;
		}

		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
		{
			FallbackEjectAndDelete();
			return;
		}

		int playerId = pm.GetPlayerIdFromControlledEntity(occupant);
		if (playerId == 0)
		{
			FallbackEjectAndDelete();
			return;
		}

		SCR_PlayerController pc = SCR_PlayerController.Cast(pm.GetPlayerController(playerId));
		if (!pc)
		{
			FallbackEjectAndDelete();
			return;
		}

		ParachuteComponent parachuteComp = ParachuteComponent.Cast(pc.FindComponent(ParachuteComponent));
		if (!parachuteComp)
		{
			FallbackEjectAndDelete();
			return;
		}

		parachuteComp.Rpc_ServerExitParachute(GetRplId(), velocityAtExit);
	}

	override void EOnContact(IEntity owner, IEntity other, Contact contact)
	{
		if (!m_RplComponent)
			return;

		if (!IsAuthority() && !IsOwner())
			return;

		if (m_bHasLanded)
			return;

		if (!m_Compartment || !m_Compartment.IsOccupied())
			return;

		if (other == m_Pilot)
			return;

		if (!ParachuteHelperFunctions.IsWithinTerrainContactThreshold(contact.Position, m_fTerrainContactThresholdM))
			return;

		AskServerExit();
	}

	override void SetPitch(float value = 0.0, EActionTrigger reason = 0, string actionName = string.Empty)
	{
		if (m_bDeployInvincibilityActive)
			return;
		super.SetPitch(value, reason, actionName);
	}

	override void SetRoll(float value = 0.0, EActionTrigger reason = 0, string actionName = string.Empty)
	{
		if (m_bDeployInvincibilityActive)
			return;
		super.SetRoll(value, reason, actionName);
	}

	override void DestroyParachute()
	{
		if (m_bIsDestroyed)
			return;

		m_bIsDestroyed = true;
		ClearInvincibilityPhysicsBlock();

		if (m_Compartment && m_Compartment.IsOccupied())
		{
			AskServerExit();
			// Do NOT delete here - player is parented to chute. Rpc_ServerExitParachute will
			// unparent the player via AskOwnerToGetOutFromVehicle, then delete the chute.
		}
		else
		{
			EjectOccupantIfAny();
			if (GetGame())
				GetGame().GetCallqueue().CallLater(ParachuteHelperFunctions.DeleteEntityIfValid, m_iEmptyChuteDeleteDelayMs, false, this);
		}
	}
}
