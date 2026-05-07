//------------------------------------------------------------------------------------------------
//! Hold + scroll to cycle IFF number while beacon is OFF.
class HMD_IffAttachableBeaconScrollNumberAction : HMD_IffAttachableBeaconScrollActionBase
{
	//------------------------------------------------------------------------------------------------
	override protected float GetScrollNormalized01()
	{
		if (!m_pBeacon)
			return 0.0;
		return m_pBeacon.GetNumberNormalized01();
	}

	//------------------------------------------------------------------------------------------------
	override protected float GetScrollNormalized01AfterStep(int dir)
	{
		if (!m_pBeacon)
			return 0.0;
		return m_pBeacon.PredictNumberNormalized01AfterDir(dir);
	}

	//------------------------------------------------------------------------------------------------
	override protected void ApplyServerScrollFromNormalized(float normalized01)
	{
		HMD_IffAttachableBeaconComponent b = ResolveBeaconForName();
		if (b)
			b.ServerApplyNumberFromNormalized01(normalized01);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnScrollDirection(int dir)
	{
		HMD_IffAttachableBeaconComponent b = ResolveBeaconForName();
		if (b)
			b.TryCycleNumberDirection(dir);
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		HMD_IffAttachableBeaconComponent b = ResolveBeaconForName();
		if (!b)
		{
			outName = "IFF number (hold + scroll)";
			return true;
		}
		outName = string.Format("IFF number: %1 (hold + scroll)", b.GetPreviewNumberString());
		return true;
	}
}
