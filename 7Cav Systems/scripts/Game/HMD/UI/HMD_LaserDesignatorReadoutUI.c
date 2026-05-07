//------------------------------------------------------------------------------------------------
//! Binds widgets from handheld (red) and vehicle-turret (white) readout layouts.
class HMD_LaserDesignatorReadoutUI
{
	protected static Widget s_wRoot;
	protected static TextWidget s_wRange;
	protected static TextWidget s_wGrid;
	protected static TextWidget s_wBearing;
	protected static TextWidget s_wCode;

	protected static Widget s_wVehicleTurretRoot;
	protected static TextWidget s_wVehicleRange;
	protected static TextWidget s_wVehicleGrid;
	protected static TextWidget s_wVehicleBearing;
	protected static TextWidget s_wVehicleCode;

	static void BindHandheldFromLayoutRoot(Widget root)
	{
		if (!root)
			return;
		s_wRoot = root;
		s_wRange = TextWidget.Cast(root.FindAnyWidget("HMD_RangefinderRange"));
		s_wGrid = TextWidget.Cast(root.FindAnyWidget("HMD_RangefinderGrid"));
		s_wBearing = TextWidget.Cast(root.FindAnyWidget("HMD_RangefinderBearing"));
		s_wCode = TextWidget.Cast(root.FindAnyWidget("HMD_RangefinderCode"));
	}

	static void BindVehicleTurretFromLayoutRoot(Widget root)
	{
		if (!root)
			return;
		s_wVehicleTurretRoot = root;
		s_wVehicleRange = TextWidget.Cast(root.FindAnyWidget("HMD_RangefinderRange"));
		s_wVehicleGrid = TextWidget.Cast(root.FindAnyWidget("HMD_RangefinderGrid"));
		s_wVehicleBearing = TextWidget.Cast(root.FindAnyWidget("HMD_RangefinderBearing"));
		s_wVehicleCode = TextWidget.Cast(root.FindAnyWidget("HMD_RangefinderCode"));
	}

	static void ClearBindings()
	{
		s_wRoot = null;
		s_wRange = null;
		s_wGrid = null;
		s_wBearing = null;
		s_wCode = null;
		s_wVehicleTurretRoot = null;
		s_wVehicleRange = null;
		s_wVehicleGrid = null;
		s_wVehicleBearing = null;
		s_wVehicleCode = null;
	}

	static TextWidget GetRangeWidget()
	{
		return s_wRange;
	}

	static TextWidget GetGridWidget()
	{
		return s_wGrid;
	}

	static TextWidget GetBearingWidget()
	{
		return s_wBearing;
	}

	static TextWidget GetCodeWidget()
	{
		return s_wCode;
	}

	static TextWidget GetVehicleTurretRangeWidget()
	{
		return s_wVehicleRange;
	}

	static TextWidget GetVehicleTurretGridWidget()
	{
		return s_wVehicleGrid;
	}

	static TextWidget GetVehicleTurretBearingWidget()
	{
		return s_wVehicleBearing;
	}

	static TextWidget GetVehicleTurretCodeWidget()
	{
		return s_wVehicleCode;
	}
}
