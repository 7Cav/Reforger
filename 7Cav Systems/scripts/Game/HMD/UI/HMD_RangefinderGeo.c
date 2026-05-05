//------------------------------------------------------------------------------------------------
//! Map grid (8-digit style) and horizontal bearing for laser designator HUD readout.
class HMD_RangefinderGeo
{
	protected const float RAD_TO_DEG = 57.29577951308232;

	//------------------------------------------------------------------------------------------------
	static string FormatEightDigitGrid(vector worldPos)
	{
		int gx, gz;
		SCR_MapEntity.GetGridPos(worldPos, gx, gz, 1, 4);
		return string.Format("%1 %2", Pad4(gx), Pad4(gz));
	}

	//------------------------------------------------------------------------------------------------
	static float BearingDegCameraToTarget(vector cameraPos, vector targetPos)
	{
		vector d = targetPos - cameraPos;
		d[1] = 0;
		if (d.Length() < 0.001)
			return 0;
		float rad = Math.Atan2(d[0], d[2]);
		float deg = rad * RAD_TO_DEG;
		if (deg < 0)
			deg += 360;
		if (deg >= 360)
			deg -= 360;
		return deg;
	}

	//------------------------------------------------------------------------------------------------
	protected static string Pad4(int n)
	{
		int v = ((n % 10000) + 10000) % 10000;
		string s = v.ToString();
		while (s.Length() < 4)
			s = "0" + s;
		return s;
	}
}
