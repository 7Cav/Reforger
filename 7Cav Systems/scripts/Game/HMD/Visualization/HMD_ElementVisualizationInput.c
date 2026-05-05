//------------------------------------------------------------------------------------------------
//! Registers legacy HMD_* input actions: layer toggles, handheld laser designate, and laser code steps.
class HMD_ElementVisualizationInput
{
	protected static bool s_bRegistered;
	protected static bool s_bShowIffInformational = true;
	protected static bool s_bShowNonIffInformational = true;
	protected static bool s_bShowDesignations = true;
	protected static bool s_bHmdLastLocalMountedInVehicle;

	//------------------------------------------------------------------------------------------------
	//! Workbench replays reuse script statics; `InputManager` may be replaced while `s_bRegistered` stayed true.
	//! Layer toggles then stop updating `s_bShow*` flags until listeners are bound again.
	static void ResetForNewPlaySession()
	{
		HMD_PoolMirrorSubscription.ResetForNewPlaySession();
		HMD_HmdDebug.ResetForNewPlaySession();
		Game game = GetGame();
		InputManager im;
		if (game)
			im = game.GetInputManager();
		else
			im = null;
		if (im && s_bRegistered)
		{
			im.RemoveActionListener("HMD_HUDIffMarkersToggle", EActionTrigger.DOWN, OnHudIffMarkersToggle);
			im.RemoveActionListener("HMD_HUDLaserVisibilityToggle", EActionTrigger.DOWN, OnHudLaserVisibilityToggle);
			im.RemoveActionListener("HMD_HUDLaserMarkingToggle", EActionTrigger.DOWN, OnHudLaserMarkingToggle);
			im.RemoveActionListener("HMD_LaserDesignate", EActionTrigger.DOWN, OnLaserDesignate);
			im.RemoveActionListener("HMD_LaserCodeIncrement", EActionTrigger.DOWN, OnLaserCodeInc);
			im.RemoveActionListener("HMD_LaserCodeDecrement", EActionTrigger.DOWN, OnLaserCodeDec);
			im.RemoveActionListener("HMD_VehicleLaserCodeIncrement", EActionTrigger.DOWN, OnVehicleLaserCodeInc);
			im.RemoveActionListener("HMD_VehicleLaserCodeDecrement", EActionTrigger.DOWN, OnVehicleLaserCodeDec);
			im.RemoveActionListener("HMD_LaserReferenceModeToggle", EActionTrigger.DOWN, OnLaserReferenceModeToggle);
		}
		s_bRegistered = false;
		s_bShowIffInformational = true;
		s_bShowNonIffInformational = true;
		s_bShowDesignations = true;
		s_bHmdLastLocalMountedInVehicle = false;
	}

	//------------------------------------------------------------------------------------------------
	//! Informational pool class **0** (IFF). While mounted, class **1–2** rows whose `m_Parent0` lies under the vehicle's
	//! `HMD_LaserMarkingRemoteVehicleComponent` owner also follow this flag (numpad 9 with WP/RP).
	static bool ShowIffInformationalMarkers()
	{
		return s_bShowIffInformational;
	}

	//------------------------------------------------------------------------------------------------
	//! Informational pool rows with class 1+ (waypoint, reference, generic).
	static bool ShowNonIffInformationalMarkers()
	{
		return s_bShowNonIffInformational;
	}

	//------------------------------------------------------------------------------------------------
	//! Class 0: IFF layer. Class 1–2: vehicle-local WP/RP (parent `RplId` under `HMD_LaserMarkingRemoteVehicleComponent` owner) use the **same** flag as IFF (numpad 9); other non-IFF uses the foot / global layer.
	static bool ShowInformationalMirrorRow(HMD_PoolMirrorRow row)
	{
		if (!row)
			return false;
		int cls = row.m_iClassType;
		if (cls == 0)
			return s_bShowIffInformational;
		if (cls == 1 || cls == 2)
		{
			if (HMD_EntityHmdHelpers.IsLocalMountedInVehicle())
			{
				IEntity ch = SCR_PlayerController.GetLocalMainEntity();
				if (!ch)
					ch = SCR_PlayerController.GetLocalControlledEntity();
				if (ch)
				{
					IEntity root = ch.GetRootParent();
					if (root && root != ch)
					{
						if (HMD_LaserMarkingRemoteVehicleComponent.HmdLocalVehiclePoolRowParentMatchesRemoteMarkingHierarchy(root, row.m_Parent0))
							return s_bShowIffInformational;
					}
				}
			}
			return s_bShowNonIffInformational;
		}
		return s_bShowNonIffInformational;
	}

	//------------------------------------------------------------------------------------------------
	static bool ShowTargetDesignations()
	{
		return s_bShowDesignations;
	}

	//------------------------------------------------------------------------------------------------
	//! True when the local crewed vehicle has laser marking (`HMD_LaserMarkingCoreComponent`) and marking is toggled on (replicated).
	static bool LocalVehicleLaserMarkingActiveForOwnLaserHudBypass()
	{
		if (!HMD_EntityHmdHelpers.IsLocalMountedInVehicle())
			return false;
		IEntity ch = SCR_PlayerController.GetLocalMainEntity();
		if (!ch)
			ch = SCR_PlayerController.GetLocalControlledEntity();
		if (!ch)
			return false;
		IEntity root = ch.GetRootParent();
		if (!root || root == ch)
			return false;
		HMD_LaserMarkingCoreComponent lm = HMD_EntityHmdHelpers.FindFirstLaserMarkingInHierarchy(root, 12);
		if (!lm)
			return false;
		return lm.HmdIsLaserMarkingActive();
	}

	//------------------------------------------------------------------------------------------------
	//! Client: sync pool mirror subscription tokens (vehicle HMD seat + handheld designator viewport).
	static void PollPoolMirrorSubscription()
	{
		HMD_PoolMirrorSubscription.PollBuiltinEligibilityAndSyncTokens();
	}

	//------------------------------------------------------------------------------------------------
	//! When the local pawn becomes mounted, hide all HMD pool HUD layers and turn off vehicle laser marking (server).
	static void PollLocalVehicleMountTransition()
	{
		bool mounted = HMD_EntityHmdHelpers.IsLocalMountedInVehicle();
		if (mounted && !s_bHmdLastLocalMountedInVehicle)
		{
			s_bShowIffInformational = false;
			s_bShowNonIffInformational = false;
			s_bShowDesignations = false;
			IEntity ch = SCR_PlayerController.GetLocalMainEntity();
			if (!ch)
				ch = SCR_PlayerController.GetLocalControlledEntity();
			if (ch)
			{
				IEntity root = ch.GetRootParent();
				if (root && root != ch)
				{
					HMD_LaserMarkingRemoteVehicleComponent rm = HMD_LaserMarkingRemoteVehicleComponent.HmdResolveLocalPlayersRemoteMarkingComponent();
					if (!rm)
						rm = HMD_EntityHmdHelpers.FindFirstLaserMarkingRemoteInHierarchy(root, 24);
					if (rm)
						rm.AskSetLaserReferenceModeFromLocal(false);
					HMD_LaserMarkingCoreComponent lm = HMD_EntityHmdHelpers.FindFirstLaserMarkingInHierarchy(root, 12);
					if (lm)
						lm.AskSetLaserMarkingActiveFromLocal(false);
				}
			}
		}
		s_bHmdLastLocalMountedInVehicle = mounted;
	}

	//------------------------------------------------------------------------------------------------
	static void RegisterOnce()
	{
		if (s_bRegistered)
			return;
		Game game = GetGame();
		if (!game)
			return;
		InputManager im = game.GetInputManager();
		if (!im)
			return;
		im.RemoveActionListener("HMD_HUDIffMarkersToggle", EActionTrigger.DOWN, OnHudIffMarkersToggle);
		im.AddActionListener("HMD_HUDIffMarkersToggle", EActionTrigger.DOWN, OnHudIffMarkersToggle);
		im.RemoveActionListener("HMD_HUDLaserVisibilityToggle", EActionTrigger.DOWN, OnHudLaserVisibilityToggle);
		im.AddActionListener("HMD_HUDLaserVisibilityToggle", EActionTrigger.DOWN, OnHudLaserVisibilityToggle);
		im.RemoveActionListener("HMD_HUDLaserMarkingToggle", EActionTrigger.DOWN, OnHudLaserMarkingToggle);
		im.AddActionListener("HMD_HUDLaserMarkingToggle", EActionTrigger.DOWN, OnHudLaserMarkingToggle);
		im.RemoveActionListener("HMD_LaserDesignate", EActionTrigger.DOWN, OnLaserDesignate);
		im.AddActionListener("HMD_LaserDesignate", EActionTrigger.DOWN, OnLaserDesignate);
		im.RemoveActionListener("HMD_LaserCodeIncrement", EActionTrigger.DOWN, OnLaserCodeInc);
		im.AddActionListener("HMD_LaserCodeIncrement", EActionTrigger.DOWN, OnLaserCodeInc);
		im.RemoveActionListener("HMD_LaserCodeDecrement", EActionTrigger.DOWN, OnLaserCodeDec);
		im.AddActionListener("HMD_LaserCodeDecrement", EActionTrigger.DOWN, OnLaserCodeDec);
		im.RemoveActionListener("HMD_VehicleLaserCodeIncrement", EActionTrigger.DOWN, OnVehicleLaserCodeInc);
		im.AddActionListener("HMD_VehicleLaserCodeIncrement", EActionTrigger.DOWN, OnVehicleLaserCodeInc);
		im.RemoveActionListener("HMD_VehicleLaserCodeDecrement", EActionTrigger.DOWN, OnVehicleLaserCodeDec);
		im.AddActionListener("HMD_VehicleLaserCodeDecrement", EActionTrigger.DOWN, OnVehicleLaserCodeDec);
		im.RemoveActionListener("HMD_LaserReferenceModeToggle", EActionTrigger.DOWN, OnLaserReferenceModeToggle);
		im.AddActionListener("HMD_LaserReferenceModeToggle", EActionTrigger.DOWN, OnLaserReferenceModeToggle);
		s_bRegistered = true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void OnHudIffMarkersToggle(float value, EActionTrigger reason)
	{
		if (!HMD_EntityHmdHelpers.IsLocalEligibleForHmdHudToggleHints())
			return;
		s_bShowIffInformational = !s_bShowIffInformational;
		HMD_HmdDebug.CliEligibility(string.Format("Layer toggle: IFF + vehicle-local WP/RP (informational class 0–2 under remote marking) -> %1", s_bShowIffInformational));
	}

	//------------------------------------------------------------------------------------------------
	protected static void OnHudLaserVisibilityToggle(float value, EActionTrigger reason)
	{
		if (!HMD_EntityHmdHelpers.IsLocalEligibleForHmdHudToggleHints())
			return;
		s_bShowDesignations = !s_bShowDesignations;
		HMD_HmdDebug.CliEligibility(string.Format("Layer toggle: target designations HUD -> %1", s_bShowDesignations));
	}

	//------------------------------------------------------------------------------------------------
	protected static void OnHudLaserMarkingToggle(float value, EActionTrigger reason)
	{
		//! `HMD_HUDLaserMarkingToggle` defaults to **KC_DIVIDE** (numpad `/` on US keyboards) in `chimeraInputCommon.conf`.
		if (HMD_EntityHmdHelpers.IsLocalHeldHmdLaserDesignatorUsingAds())
			return;
		if (HMD_EntityHmdHelpers.IsLocalMountedInVehicle())
		{
			if (!HMD_EntityHmdHelpers.LocalVehicleRootHasLaserMarkingCapability(12))
				return;
			IEntity chEnt = SCR_PlayerController.GetLocalMainEntity();
			if (!chEnt)
				chEnt = SCR_PlayerController.GetLocalControlledEntity();
			IEntity root = chEnt.GetRootParent();
			if (!root || root == chEnt)
				return;
			HMD_LaserMarkingRemoteVehicleComponent rm = HMD_LaserMarkingRemoteVehicleComponent.HmdResolveLocalPlayersRemoteMarkingComponent();
			HMD_LaserMarkingCoreComponent lm = rm;
			if (!lm)
				lm = HMD_EntityHmdHelpers.FindFirstLaserMarkingInHierarchy(root, 12);
			if (!lm || !lm.HmdIsLocalPlayerEligibleForLaserControl())
				return;
			if (rm && rm.HmdIsLaserReferenceMode())
			{
				rm.AskPlaceRpMarkerFromLocal();
				HMD_HmdDebug.CliLaserRef("Vehicle laser reference: place RP (numpad /)");
				return;
			}
			lm.AskToggleLaserMarkingFromLocal();
			HMD_HmdDebug.CliEligibility("Vehicle laser marking: toggle requested (numpad HUD bind)");
			return;
		}
		if (!HMD_EntityHmdHelpers.IsLocalEligibleForHmdHudToggleHints())
			return;
		s_bShowNonIffInformational = !s_bShowNonIffInformational;
		HMD_HmdDebug.CliEligibility(string.Format("Layer toggle: non-IFF informational markers -> %1", s_bShowNonIffInformational));
	}

	//------------------------------------------------------------------------------------------------
	protected static HMD_DesignationElementBaseComponent FindHeldDesignationForLocal()
	{
		IEntity local = SCR_PlayerController.GetLocalMainEntity();
		if (!local)
			return null;
		SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(local);
		if (!gm)
			return null;
		return HMD_DesignationElementBaseComponent.Cast(gm.GetHeldGadgetComponent());
	}

	//------------------------------------------------------------------------------------------------
	protected static HMD_LaserMarkingHandheldComponent FindHeldHandheldLaserMarkingForLocal()
	{
		IEntity local = SCR_PlayerController.GetLocalMainEntity();
		if (!local)
			return null;
		SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(local);
		if (!gm)
			return null;
		SCR_GadgetComponent held = gm.GetHeldGadgetComponent();
		if (!held)
			return null;
		IEntity gadgetEnt = held.GetOwner();
		if (!gadgetEnt)
			return null;
		return HMD_LaserMarkingHandheldComponent.Cast(gadgetEnt.FindComponent(HMD_LaserMarkingHandheldComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected static void OnLaserDesignate(float value, EActionTrigger reason)
	{
		if (!HMD_EntityHmdHelpers.IsLocalHeldHmdLaserDesignatorUsingAds())
			return;
		HMD_LaserMarkingHandheldComponent hlm = FindHeldHandheldLaserMarkingForLocal();
		if (hlm)
		{
			hlm.AskToggleLaserMarkingFromLocal();
			return;
		}
		HMD_DesignationElementBaseComponent des = FindHeldDesignationForLocal();
		if (des)
			des.HmdToggleDesignatingForInput();
	}

	//------------------------------------------------------------------------------------------------
	protected static void OnLaserCodeInc(float value, EActionTrigger reason)
	{
		if (!HMD_EntityHmdHelpers.IsLocalHeldHmdLaserDesignatorUsingAds())
			return;
		HMD_LaserMarkingHandheldComponent hlm = FindHeldHandheldLaserMarkingForLocal();
		if (hlm)
		{
			hlm.AskStepActiveDesignationLaserCodeFromLocal(1);
			return;
		}
		HMD_DesignationElementBaseComponent des = FindHeldDesignationForLocal();
		if (des)
			des.AskStepLaserCodeFromOwner(1);
	}

	//------------------------------------------------------------------------------------------------
	protected static void OnLaserCodeDec(float value, EActionTrigger reason)
	{
		if (!HMD_EntityHmdHelpers.IsLocalHeldHmdLaserDesignatorUsingAds())
			return;
		HMD_LaserMarkingHandheldComponent hlm = FindHeldHandheldLaserMarkingForLocal();
		if (hlm)
		{
			hlm.AskStepActiveDesignationLaserCodeFromLocal(-1);
			return;
		}
		HMD_DesignationElementBaseComponent des = FindHeldDesignationForLocal();
		if (des)
			des.AskStepLaserCodeFromOwner(-1);
	}

	//------------------------------------------------------------------------------------------------
	protected static void OnVehicleLaserCodeInc(float value, EActionTrigger reason)
	{
		if (HMD_EntityHmdHelpers.IsLocalHeldHmdLaserDesignatorUsingAds())
			return;
		if (!HMD_EntityHmdHelpers.IsLocalMountedInVehicle())
			return;
		if (!HMD_EntityHmdHelpers.LocalVehicleRootHasLaserMarkingCapability(12))
			return;
		IEntity ch = SCR_PlayerController.GetLocalMainEntity();
		if (!ch)
			ch = SCR_PlayerController.GetLocalControlledEntity();
		if (!ch)
			return;
		IEntity root = ch.GetRootParent();
		if (!root || root == ch)
			return;
		HMD_LaserMarkingRemoteVehicleComponent rm = HMD_LaserMarkingRemoteVehicleComponent.HmdResolveLocalPlayersRemoteMarkingComponent();
		HMD_LaserMarkingCoreComponent lm = rm;
		if (!lm)
			lm = HMD_EntityHmdHelpers.FindFirstLaserMarkingInHierarchy(root, 16);
		if (!lm || !lm.HmdIsLocalPlayerEligibleForLaserControl())
			return;
		if (rm && rm.HmdIsLaserReferenceMode())
		{
			rm.AskPlaceWpMarkerFromLocal();
			return;
		}
		lm.AskStepActiveDesignationLaserCodeFromLocal(1);
	}

	//------------------------------------------------------------------------------------------------
	protected static void OnVehicleLaserCodeDec(float value, EActionTrigger reason)
	{
		if (HMD_EntityHmdHelpers.IsLocalHeldHmdLaserDesignatorUsingAds())
			return;
		if (!HMD_EntityHmdHelpers.IsLocalMountedInVehicle())
			return;
		if (!HMD_EntityHmdHelpers.LocalVehicleRootHasLaserMarkingCapability(12))
			return;
		IEntity ch = SCR_PlayerController.GetLocalMainEntity();
		if (!ch)
			ch = SCR_PlayerController.GetLocalControlledEntity();
		if (!ch)
			return;
		IEntity root = ch.GetRootParent();
		if (!root || root == ch)
			return;
		HMD_LaserMarkingRemoteVehicleComponent rm = HMD_LaserMarkingRemoteVehicleComponent.HmdResolveLocalPlayersRemoteMarkingComponent();
		HMD_LaserMarkingCoreComponent lm = rm;
		if (!lm)
			lm = HMD_EntityHmdHelpers.FindFirstLaserMarkingInHierarchy(root, 16);
		if (!lm || !lm.HmdIsLocalPlayerEligibleForLaserControl())
			return;
		if (rm && rm.HmdIsLaserReferenceMode())
		{
			rm.AskDeleteScopedMarkerFromLocal();
			return;
		}
		lm.AskStepActiveDesignationLaserCodeFromLocal(-1);
	}

	//------------------------------------------------------------------------------------------------
	protected static void OnLaserReferenceModeToggle(float value, EActionTrigger reason)
	{
		if (HMD_EntityHmdHelpers.IsLocalHeldHmdLaserDesignatorUsingAds())
			return;
		if (!HMD_EntityHmdHelpers.IsLocalMountedInVehicle())
			return;
		if (!HMD_EntityHmdHelpers.LocalVehicleRootHasLaserMarkingCapability(12))
			return;
		IEntity ch = SCR_PlayerController.GetLocalMainEntity();
		if (!ch)
			ch = SCR_PlayerController.GetLocalControlledEntity();
		if (!ch)
			return;
		IEntity root = ch.GetRootParent();
		if (!root || root == ch)
			return;
		HMD_LaserMarkingRemoteVehicleComponent rm = HMD_LaserMarkingRemoteVehicleComponent.HmdResolveLocalPlayersRemoteMarkingComponent();
		if (!rm)
			return;
		if (!rm.HmdIsLocalPlayerEligibleForLaserControl())
			return;
		rm.AskToggleLaserReferenceModeFromLocal();
		HMD_HmdDebug.CliLaserRef("Vehicle laser reference mode: toggle (numpad 6)");
	}
}
