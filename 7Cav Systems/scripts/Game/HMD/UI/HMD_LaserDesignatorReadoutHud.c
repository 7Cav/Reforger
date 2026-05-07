//------------------------------------------------------------------------------------------------
//! Creates handheld + vehicle rangefinder readout layouts and drives them **only while laser marking is active**.
class HMD_LaserDesignatorReadoutHud
{
	protected static Widget s_wHandRoot;
	protected static Widget s_wVehRoot;

	protected static const ResourceName LAYOUT_HAND = "{6900000B00000001}UI/layouts/HUD/HMD_LaserDesignatorReadout.layout";
	protected static const ResourceName LAYOUT_VEH = "{6900000B00000002}UI/layouts/HUD/HMD_VehicleTurretLaserDesignatorReadout.layout";

	//------------------------------------------------------------------------------------------------
	static void ResetForNewPlaySession()
	{
		HMD_RangefinderHUDState.Clear();
		HMD_LaserDesignatorReadoutUI.ClearBindings();
		s_wHandRoot = null;
		s_wVehRoot = null;
	}

	//------------------------------------------------------------------------------------------------
	static void EnsureLayouts(SCR_HUDManagerComponent hud)
	{
		if (!hud)
			return;
		if (!s_wHandRoot)
		{
			s_wHandRoot = hud.CreateLayout(LAYOUT_HAND, EHudLayers.MEDIUM, 0);
			if (s_wHandRoot)
			{
				s_wHandRoot.SetVisible(false);
				HMD_LaserDesignatorReadoutUI.BindHandheldFromLayoutRoot(s_wHandRoot);
			}
		}
		if (!s_wVehRoot)
		{
			s_wVehRoot = hud.CreateLayout(LAYOUT_VEH, EHudLayers.MEDIUM, 0);
			if (s_wVehRoot)
			{
				s_wVehRoot.SetVisible(false);
				HMD_LaserDesignatorReadoutUI.BindVehicleTurretFromLayoutRoot(s_wVehRoot);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void HmdPushReadoutFromLaserMarking(HMD_LaserMarkingCoreComponent lm)
	{
		if (!lm || !lm.HmdIsLaserMarkingActive())
			return;
		vector origin;
		vector dir;
		IEntity traceHost;
		if (!lm.HmdTryGetClientHudDesignationRay(origin, dir, traceHost))
			return;
		float maxR = lm.HmdGetDesignationTraceMaxRangeM();
		vector hit;
		float frac;
		bool traced = HMD_DesignatorRayTraceUtils.TraceRay(traceHost, origin, dir, maxR, hit, frac);
		float rangeM = maxR;
		bool maxRangeExceeded = !traced;
		string gridStr = "";
		float bearingDeg = 0;
		if (traced)
		{
			rangeM = (origin - hit).Length();
			maxRangeExceeded = rangeM >= maxR - 0.25 || frac >= 0.9999;
			if (!maxRangeExceeded)
			{
				gridStr = HMD_RangefinderGeo.FormatEightDigitGrid(hit);
				bearingDeg = HMD_RangefinderGeo.BearingDegCameraToTarget(origin, hit);
			}
		}
		HMD_RangefinderHUDState.SetDesignatorCode(lm.HmdGetReplicatedLaserCode());
		HMD_RangefinderHUDState.SetLasingReadout(rangeM, gridStr, bearingDeg, maxRangeExceeded);
	}

	//------------------------------------------------------------------------------------------------
	static void Tick(SCR_HUDManagerComponent hud)
	{
		EnsureLayouts(hud);

		SCR_EditorManagerEntity edMgr = SCR_EditorManagerEntity.GetInstance();
		if (edMgr && edMgr.IsOpened())
		{
			if (s_wHandRoot)
				s_wHandRoot.SetVisible(false);
			if (s_wVehRoot)
				s_wVehRoot.SetVisible(false);
			HMD_RangefinderHUDState.Clear();
			HMD_RangefinderReadout.Apply(HMD_LaserDesignatorReadoutUI.GetRangeWidget(), HMD_LaserDesignatorReadoutUI.GetGridWidget(), HMD_LaserDesignatorReadoutUI.GetBearingWidget(), HMD_LaserDesignatorReadoutUI.GetCodeWidget(), false, false);
			HMD_RangefinderReadout.Apply(HMD_LaserDesignatorReadoutUI.GetVehicleTurretRangeWidget(), HMD_LaserDesignatorReadoutUI.GetVehicleTurretGridWidget(), HMD_LaserDesignatorReadoutUI.GetVehicleTurretBearingWidget(), HMD_LaserDesignatorReadoutUI.GetVehicleTurretCodeWidget(), false, true);
			return;
		}

		IEntity playerChar = SCR_PlayerController.GetLocalMainEntity();
		if (!playerChar)
			playerChar = SCR_PlayerController.GetLocalControlledEntity();
		if (!playerChar)
		{
			if (s_wHandRoot)
				s_wHandRoot.SetVisible(false);
			if (s_wVehRoot)
				s_wVehRoot.SetVisible(false);
			HMD_RangefinderHUDState.Clear();
			HMD_RangefinderReadout.Apply(HMD_LaserDesignatorReadoutUI.GetRangeWidget(), HMD_LaserDesignatorReadoutUI.GetGridWidget(), HMD_LaserDesignatorReadoutUI.GetBearingWidget(), HMD_LaserDesignatorReadoutUI.GetCodeWidget(), false, false);
			HMD_RangefinderReadout.Apply(HMD_LaserDesignatorReadoutUI.GetVehicleTurretRangeWidget(), HMD_LaserDesignatorReadoutUI.GetVehicleTurretGridWidget(), HMD_LaserDesignatorReadoutUI.GetVehicleTurretBearingWidget(), HMD_LaserDesignatorReadoutUI.GetVehicleTurretCodeWidget(), false, true);
			return;
		}

		SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(playerChar.FindComponent(SCR_CharacterControllerComponent));
		if (charController)
		{
			ECharacterLifeState lifeState = charController.GetLifeState();
			if (lifeState == ECharacterLifeState.DEAD || lifeState == ECharacterLifeState.INCAPACITATED)
			{
				if (s_wHandRoot)
					s_wHandRoot.SetVisible(false);
				if (s_wVehRoot)
					s_wVehRoot.SetVisible(false);
				HMD_RangefinderHUDState.Clear();
				HMD_RangefinderReadout.Apply(HMD_LaserDesignatorReadoutUI.GetRangeWidget(), HMD_LaserDesignatorReadoutUI.GetGridWidget(), HMD_LaserDesignatorReadoutUI.GetBearingWidget(), HMD_LaserDesignatorReadoutUI.GetCodeWidget(), false, false);
				HMD_RangefinderReadout.Apply(HMD_LaserDesignatorReadoutUI.GetVehicleTurretRangeWidget(), HMD_LaserDesignatorReadoutUI.GetVehicleTurretGridWidget(), HMD_LaserDesignatorReadoutUI.GetVehicleTurretBearingWidget(), HMD_LaserDesignatorReadoutUI.GetVehicleTurretCodeWidget(), false, true);
				return;
			}
		}

		bool mounted = HMD_EntityHmdHelpers.IsLocalMountedInVehicle();
		bool zoom = HMD_HandheldDesignatorOpticZoom.IsZoomedForHMD();

		HMD_LaserMarkingHandheldComponent hhComp;
		HMD_LaserMarkingCoreComponent vehLm = null;

		if (mounted)
		{
			IEntity root = playerChar.GetRootParent();
			if (root && root != playerChar)
			{
				HMD_LaserMarkingRemoteVehicleComponent rm = HMD_EntityHmdHelpers.FindFirstLaserMarkingRemoteInHierarchy(root, 24);
				if (rm && rm.HmdIsLaserReferenceMode())
					vehLm = null;
				else
					vehLm = HMD_EntityHmdHelpers.FindFirstLaserMarkingInHierarchy(root, 12);
			}
		}

		//! Same gate as laser toggle / code RPCs: only seats allowed by the marking component (not general vehicle HUD).
		bool vehLaserCtrl = vehLm && vehLm.HmdIsLocalPlayerEligibleForLaserControl();

		SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(playerChar);
		if (gm)
		{
			SCR_GadgetComponent held = gm.GetHeldGadgetComponent();
			if (held)
			{
				IEntity gadgetEnt = held.GetOwner();
				if (gadgetEnt)
					hhComp = HMD_HandheldDesignatorOpticZoom.FindHandheldLaserMarkingOnGadget(gadgetEnt);
			}
		}

		bool handLaserOn = hhComp && zoom && hhComp.HmdIsLaserMarkingActive();
		bool vehLaserOn = mounted && vehLaserCtrl && vehLm.HmdIsLaserMarkingActive();
		bool vehBinocularLaser = mounted && zoom && vehLaserCtrl && vehLm.HmdIsLaserMarkingActive();

		bool showVehicleLayout = vehLaserOn && !zoom;
		bool showHandheldLayout = handLaserOn || vehBinocularLaser;

		if (showHandheldLayout && hhComp)
			HmdPushReadoutFromLaserMarking(hhComp);
		else if (showHandheldLayout && vehLm)
			HmdPushReadoutFromLaserMarking(vehLm);
		else if (showVehicleLayout && vehLm)
			HmdPushReadoutFromLaserMarking(vehLm);

		if (s_wHandRoot)
			s_wHandRoot.SetVisible(showHandheldLayout);
		if (s_wVehRoot)
			s_wVehRoot.SetVisible(showVehicleLayout);

		HMD_RangefinderReadout.Apply(HMD_LaserDesignatorReadoutUI.GetRangeWidget(), HMD_LaserDesignatorReadoutUI.GetGridWidget(), HMD_LaserDesignatorReadoutUI.GetBearingWidget(), HMD_LaserDesignatorReadoutUI.GetCodeWidget(), showHandheldLayout, false);
		HMD_RangefinderReadout.Apply(HMD_LaserDesignatorReadoutUI.GetVehicleTurretRangeWidget(), HMD_LaserDesignatorReadoutUI.GetVehicleTurretGridWidget(), HMD_LaserDesignatorReadoutUI.GetVehicleTurretBearingWidget(), HMD_LaserDesignatorReadoutUI.GetVehicleTurretCodeWidget(), showVehicleLayout, true);
	}
}
