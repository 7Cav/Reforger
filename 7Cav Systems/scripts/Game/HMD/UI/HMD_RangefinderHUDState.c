//------------------------------------------------------------------------------------------------
//! Client-side rangefinder readout values; updated from `HMD_LaserDesignatorReadoutHud` while laser marking is active.
class HMD_RangefinderHUDState
{
	protected static float s_fRangeM;
	protected static string s_sGrid;
	protected static float s_fBearingDeg;
	protected static bool s_bLasingActive;
	protected static bool s_bMaxRangeExceeded;
	protected static bool s_bLockTargetReadout;
	protected static int s_iDesignatorCode;

	static void SetLasingReadout(float rangeM, string gridStr, float bearingDeg, bool maxRangeExceeded)
	{
		s_bLasingActive = true;
		s_fRangeM = rangeM;
		s_sGrid = gridStr;
		s_fBearingDeg = bearingDeg;
		s_bMaxRangeExceeded = maxRangeExceeded;
	}

	static void Clear()
	{
		s_bLasingActive = false;
		s_bLockTargetReadout = false;
		s_fRangeM = 0;
		s_sGrid = "";
		s_fBearingDeg = 0;
		s_bMaxRangeExceeded = false;
		s_iDesignatorCode = 0;
	}

	static void SetDesignatorCode(int code)
	{
		s_iDesignatorCode = code;
	}

	static int GetDesignatorCode()
	{
		return s_iDesignatorCode;
	}

	static bool IsLasingActive()
	{
		return s_bLasingActive;
	}

	static bool IsLockTargetReadout()
	{
		return s_bLockTargetReadout;
	}

	static void SetLockTargetReadout(float rangeM, string gridStr, float bearingDeg, int code)
	{
		s_bLockTargetReadout = true;
		s_bLasingActive = false;
		s_fRangeM = rangeM;
		s_sGrid = gridStr;
		s_fBearingDeg = bearingDeg;
		s_bMaxRangeExceeded = false;
		s_iDesignatorCode = code;
	}

	static void ClearLockTargetReadout()
	{
		if (!s_bLockTargetReadout)
			return;
		s_bLockTargetReadout = false;
		s_fRangeM = 0;
		s_sGrid = "";
		s_fBearingDeg = 0;
		s_bMaxRangeExceeded = false;
		s_iDesignatorCode = 0;
	}

	static float GetRangeM()
	{
		return s_fRangeM;
	}

	static string GetGrid()
	{
		return s_sGrid;
	}

	static float GetBearingDeg()
	{
		return s_fBearingDeg;
	}

	static bool IsMaxRangeExceeded()
	{
		return s_bMaxRangeExceeded;
	}
}
