//------------------------------------------------------------------------------------------------
//! Hold interact + scroll (`SCR_AdjustSignalAction`) while beacon is OFF; server applies discrete steps from replicated action data.
class HMD_IffAttachableBeaconScrollActionBase : SCR_AdjustSignalAction
{
	protected HMD_IffAttachableBeaconComponent m_pBeacon;

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_pBeacon = HMD_IffAttachableBeaconComponent.FindOnEntity(pOwnerEntity);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!super.CanBeShownScript(user))
			return false;
		return m_pBeacon && m_pBeacon.CanConfigure();
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	//------------------------------------------------------------------------------------------------
	override protected float SCR_GetMinimumValue()
	{
		return 0.0;
	}

	//------------------------------------------------------------------------------------------------
	override protected float SCR_GetMaximumValue()
	{
		return 1.0;
	}

	//------------------------------------------------------------------------------------------------
	override protected float SCR_GetCurrentValue()
	{
		if (!m_pBeacon)
			return 0.0;
		return GetScrollNormalized01();
	}

	//------------------------------------------------------------------------------------------------
	protected float GetScrollNormalized01()
	{
		return 0.0;
	}

	//------------------------------------------------------------------------------------------------
	protected float GetScrollNormalized01AfterStep(int dir)
	{
		return 0.0;
	}

	//------------------------------------------------------------------------------------------------
	override protected void HandleAction(float value)
	{
		if (value == 0.0)
			return;
		int dir = 1;
		if (value < 0.0)
			dir = -1;
		if (m_pBeacon)
		{
			float nextT = GetScrollNormalized01AfterStep(dir);
			OnScrollDirection(dir);
			m_fTargetValue = nextT;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnScrollDirection(int dir)
	{
	}

	//------------------------------------------------------------------------------------------------
	override protected bool OnSaveActionData(ScriptBitWriter writer)
	{
		float lerp = Math.Lerp(GetMinimumValue(), GetMaximumValue(), m_fTargetValue);
		writer.WriteFloat01(lerp);
		PlayMovementAndStopSound(lerp);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool OnLoadActionData(ScriptBitReader reader)
	{
		if (m_bIsAdjustedByPlayer)
			return true;

		float lerp;
		reader.ReadFloat01(lerp);
		m_fTargetValue = Math.InverseLerp(GetMinimumValue(), GetMaximumValue(), lerp);
		PlayMovementAndStopSound(lerp);
		if (Replication.IsRunning() && Replication.IsServer())
			ApplyServerScrollFromNormalized(m_fTargetValue);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyServerScrollFromNormalized(float normalized01)
	{
	}

	//------------------------------------------------------------------------------------------------
	protected HMD_IffAttachableBeaconComponent ResolveBeaconForName()
	{
		if (m_pBeacon)
			return m_pBeacon;
		return HMD_IffAttachableBeaconComponent.FindOnEntity(GetOwner());
	}
}
