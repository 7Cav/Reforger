//------------------------------------------------------------------------------------------------
//! Hold + scroll to cycle IFF text preset while beacon is OFF.
class HMD_IffAttachableBeaconScrollTextAction : HMD_IffAttachableBeaconScrollActionBase
{
	//------------------------------------------------------------------------------------------------
	override protected float GetScrollNormalized01()
	{
		if (!m_pBeacon)
			return 0.0;
		return m_pBeacon.GetTextIndexNormalized01();
	}

	//------------------------------------------------------------------------------------------------
	override protected float GetScrollNormalized01AfterStep(int dir)
	{
		if (!m_pBeacon)
			return 0.0;
		return m_pBeacon.PredictTextIndexNormalized01AfterDir(dir);
	}

	//------------------------------------------------------------------------------------------------
	override protected void ApplyServerScrollFromNormalized(float normalized01)
	{
		HMD_IffAttachableBeaconComponent b = ResolveBeaconForName();
		if (b)
			b.ServerApplyTextIndexFromNormalized01(normalized01);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnScrollDirection(int dir)
	{
		HMD_IffAttachableBeaconComponent b = ResolveBeaconForName();
		if (b)
			b.TryCycleTextDirection(dir);
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		HMD_IffAttachableBeaconComponent b = ResolveBeaconForName();
		if (!b)
		{
			outName = "IFF text (hold + scroll)";
			return true;
		}
		outName = string.Format("IFF text: %1 (hold + scroll)", b.GetPreviewTextCode());
		return true;
	}
}
