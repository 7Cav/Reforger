//------------------------------------------------------------------------------------------------
//! HUD opacity vs distance: full opacity below 75% of max view distance, linear fade to 0 by 100%.
class HMD_ElementVisualFadeUtils
{
	//------------------------------------------------------------------------------------------------
	static float OpacityFromDistanceM(float distM, float maxDistM)
	{
		if (maxDistM <= 0.0)
			return 1.0;
		if (distM >= maxDistM)
			return 0.0;
		float t = distM / maxDistM;
		if (t < 0.75)
			return 1.0;
		float u = (t - 0.75) / 0.25;
		float a = 1.0 - u;
		if (a < 0.0)
			return 0.0;
		if (a > 1.0)
			return 1.0;
		return a;
	}
}
