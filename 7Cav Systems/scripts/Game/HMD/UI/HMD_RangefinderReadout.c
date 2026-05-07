//------------------------------------------------------------------------------------------------
//! Fixed HUD text for rangefinder range + 8-digit grid + bearing (placeholder when no valid trace).
class HMD_RangefinderReadout
{
	static const int DESIGNATOR_READOUT_FONT_SIZE = 28;

	static bool s_bStyleAppliedHandheld;
	static bool s_bStyleAppliedVehicle;
	static bool s_bLastHudContextVisibleHandheld;
	static bool s_bLastHudContextVisibleVehicle;
	static string s_sLastRangeHandheld;
	static string s_sLastGridHandheld;
	static string s_sLastBearingHandheld;
	static string s_sLastCodeHandheld;
	static string s_sLastRangeVehicle;
	static string s_sLastGridVehicle;
	static string s_sLastBearingVehicle;
	static string s_sLastCodeVehicle;

	//------------------------------------------------------------------------------------------------
	static void Apply(TextWidget rangeW, TextWidget gridW, TextWidget bearingW, TextWidget codeW, bool hudContextVisible, bool vehicleTurretLayout)
	{
		if (!rangeW || !gridW || !bearingW || !codeW)
			return;

		if (vehicleTurretLayout)
		{
			if (!hudContextVisible)
			{
				if (s_bLastHudContextVisibleVehicle)
				{
					rangeW.SetVisible(false);
					gridW.SetVisible(false);
					bearingW.SetVisible(false);
					codeW.SetVisible(false);
					s_bLastHudContextVisibleVehicle = false;
				}
				return;
			}

			if (!s_bLastHudContextVisibleVehicle)
			{
				rangeW.SetVisible(true);
				gridW.SetVisible(true);
				bearingW.SetVisible(true);
				codeW.SetVisible(true);
				s_bLastHudContextVisibleVehicle = true;
				s_sLastRangeVehicle = "";
				s_sLastGridVehicle = "";
				s_sLastBearingVehicle = "";
				s_sLastCodeVehicle = "";
			}

			if (!s_bStyleAppliedVehicle)
			{
				Color white = Color.FromRGBA(255, 255, 255, 255);
				rangeW.SetExactFontSize(DESIGNATOR_READOUT_FONT_SIZE);
				rangeW.SetColor(white);
				gridW.SetExactFontSize(DESIGNATOR_READOUT_FONT_SIZE);
				gridW.SetColor(white);
				bearingW.SetExactFontSize(DESIGNATOR_READOUT_FONT_SIZE);
				bearingW.SetColor(white);
				codeW.SetExactFontSize(DESIGNATOR_READOUT_FONT_SIZE);
				codeW.SetColor(white);
				s_bStyleAppliedVehicle = true;
			}

			string c;
			if (HMD_RangefinderHUDState.GetDesignatorCode() > 0)
				c = string.Format("%1", HMD_RangefinderHUDState.GetDesignatorCode());
			else
				c = "----";

			string r;
			string g;
			string b;
			if (!HMD_RangefinderHUDState.IsLasingActive() && !HMD_RangefinderHUDState.IsLockTargetReadout())
			{
				r = "----";
				g = "----";
				b = "----";
			}
			else if (HMD_RangefinderHUDState.IsMaxRangeExceeded())
			{
				r = "XXXX m";
				g = "XXXX XXXX";
				b = "XXXX";
			}
			else
			{
				r = string.Format("%1 m", Math.Round(HMD_RangefinderHUDState.GetRangeM()));
				g = HMD_RangefinderHUDState.GetGrid();
				b = string.Format("%1 deg", Math.Round(HMD_RangefinderHUDState.GetBearingDeg()));
			}

			if (r != s_sLastRangeVehicle)
			{
				rangeW.SetText(r);
				s_sLastRangeVehicle = r;
			}
			if (g != s_sLastGridVehicle)
			{
				gridW.SetText(g);
				s_sLastGridVehicle = g;
			}
			if (b != s_sLastBearingVehicle)
			{
				bearingW.SetText(b);
				s_sLastBearingVehicle = b;
			}
			if (c != s_sLastCodeVehicle)
			{
				codeW.SetText(c);
				s_sLastCodeVehicle = c;
			}
			return;
		}

		if (!hudContextVisible)
		{
			if (s_bLastHudContextVisibleHandheld)
			{
				rangeW.SetVisible(false);
				gridW.SetVisible(false);
				bearingW.SetVisible(false);
				codeW.SetVisible(false);
				s_bLastHudContextVisibleHandheld = false;
			}
			return;
		}

		if (!s_bLastHudContextVisibleHandheld)
		{
			rangeW.SetVisible(true);
			gridW.SetVisible(true);
			bearingW.SetVisible(true);
			codeW.SetVisible(true);
			s_bLastHudContextVisibleHandheld = true;
			s_sLastRangeHandheld = "";
			s_sLastGridHandheld = "";
			s_sLastBearingHandheld = "";
			s_sLastCodeHandheld = "";
		}

		if (!s_bStyleAppliedHandheld)
		{
			Color white = Color.FromRGBA(255, 255, 255, 255);
			rangeW.SetExactFontSize(DESIGNATOR_READOUT_FONT_SIZE);
			rangeW.SetColor(white);
			gridW.SetExactFontSize(DESIGNATOR_READOUT_FONT_SIZE);
			gridW.SetColor(white);
			bearingW.SetExactFontSize(DESIGNATOR_READOUT_FONT_SIZE);
			bearingW.SetColor(white);
			codeW.SetExactFontSize(DESIGNATOR_READOUT_FONT_SIZE);
			codeW.SetColor(white);
			s_bStyleAppliedHandheld = true;
		}

		string c2;
		if (HMD_RangefinderHUDState.GetDesignatorCode() > 0)
			c2 = string.Format("%1", HMD_RangefinderHUDState.GetDesignatorCode());
		else
			c2 = "----";

		string r2;
		string g2;
		string b2;
		if (!HMD_RangefinderHUDState.IsLasingActive() && !HMD_RangefinderHUDState.IsLockTargetReadout())
		{
			r2 = "----";
			g2 = "----";
			b2 = "----";
		}
		else if (HMD_RangefinderHUDState.IsMaxRangeExceeded())
		{
			r2 = "XXXX m";
			g2 = "XXXX XXXX";
			b2 = "XXXX";
		}
		else
		{
			r2 = string.Format("%1 m", Math.Round(HMD_RangefinderHUDState.GetRangeM()));
			g2 = HMD_RangefinderHUDState.GetGrid();
			b2 = string.Format("%1 deg", Math.Round(HMD_RangefinderHUDState.GetBearingDeg()));
		}

		if (r2 != s_sLastRangeHandheld)
		{
			rangeW.SetText(r2);
			s_sLastRangeHandheld = r2;
		}
		if (g2 != s_sLastGridHandheld)
		{
			gridW.SetText(g2);
			s_sLastGridHandheld = g2;
		}
		if (b2 != s_sLastBearingHandheld)
		{
			bearingW.SetText(b2);
			s_sLastBearingHandheld = b2;
		}
		if (c2 != s_sLastCodeHandheld)
		{
			codeW.SetText(c2);
			s_sLastCodeHandheld = c2;
		}
	}
}
