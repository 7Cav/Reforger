//------------------------------------------------------------------------------------------------
//! Finds HMD designation components in an entity subtree (vehicle stack, character, etc.).
class HMD_EntityHmdHelpers
{
	//------------------------------------------------------------------------------------------------
	static HMD_DesignationElementBaseComponent FindHeldDesignationOnCharacter(IEntity character)
	{
		if (!character)
			return null;
		SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(character);
		if (gm)
		{
			HMD_DesignationElementBaseComponent d = HMD_DesignationElementBaseComponent.Cast(gm.GetHeldGadgetComponent());
			if (d)
				return d;
		}
		return FindFirstDesignationInHierarchy(character, 6);
	}

	//------------------------------------------------------------------------------------------------
	static HMD_DesignationElementBaseComponent FindFirstDesignationInHierarchy(IEntity root, int maxDepth)
	{
		if (!root || maxDepth < 0)
			return null;
		HMD_DesignationElementBaseComponent direct = HMD_DesignationElementBaseComponent.Cast(root.FindComponent(HMD_DesignationElementBaseComponent));
		if (direct)
			return direct;
		IEntity child = root.GetChildren();
		while (child)
		{
			HMD_DesignationElementBaseComponent found = FindFirstDesignationInHierarchy(child, maxDepth - 1);
			if (found)
				return found;
			child = child.GetSibling();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Depth-first search for a component type (same pattern as LaserFixesAgain / vehicle HUD eligibility).
	static GenericComponent FindComponentInHierarchy(IEntity root, typename componentType)
	{
		if (!root)
			return null;
		GenericComponent component = GenericComponent.Cast(root.FindComponent(componentType));
		if (component)
			return component;
		IEntity child = root.GetChildren();
		while (child)
		{
			component = FindComponentInHierarchy(child, componentType);
			if (component)
				return component;
			child = child.GetSibling();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Any vehicle / handheld specialization of `HMD_LaserMarkingCoreComponent` (Workbench uses concrete subclasses).
	static HMD_LaserMarkingCoreComponent HmdCastAnyLaserMarkingOnEntity(IEntity ent)
	{
		if (!ent)
			return null;
		return HMD_LaserMarkingCoreComponent.Cast(ent.FindComponent(HMD_LaserMarkingCoreComponent));
	}

	//------------------------------------------------------------------------------------------------
	static HMD_LaserMarkingCoreComponent FindFirstLaserMarkingInHierarchy(IEntity root, int maxDepth)
	{
		if (!root || maxDepth < 0)
			return null;
		HMD_LaserMarkingCoreComponent direct = HmdCastAnyLaserMarkingOnEntity(root);
		if (direct)
			return direct;
		IEntity child = root.GetChildren();
		while (child)
		{
			HMD_LaserMarkingCoreComponent found = FindFirstLaserMarkingInHierarchy(child, maxDepth - 1);
			if (found)
				return found;
			child = child.GetSibling();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	static HMD_LaserMarkingRemoteVehicleComponent FindFirstLaserMarkingRemoteInHierarchy(IEntity root, int maxDepth)
	{
		if (!root || maxDepth < 0)
			return null;
		HMD_LaserMarkingRemoteVehicleComponent direct = HMD_LaserMarkingRemoteVehicleComponent.Cast(root.FindComponent(HMD_LaserMarkingRemoteVehicleComponent));
		if (direct)
			return direct;
		IEntity child = root.GetChildren();
		while (child)
		{
			HMD_LaserMarkingRemoteVehicleComponent found = FindFirstLaserMarkingRemoteInHierarchy(child, maxDepth - 1);
			if (found)
				return found;
			child = child.GetSibling();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! True when the local pawn is crewed on a vehicle whose root hierarchy contains laser marking (`HMD_LaserMarkingCoreComponent`).
	static bool LocalVehicleRootHasLaserMarkingCapability(int maxDepth)
	{
		if (!IsLocalMountedInVehicle())
			return false;
		IEntity pawn = SCR_PlayerController.GetLocalMainEntity();
		if (!pawn)
			pawn = SCR_PlayerController.GetLocalControlledEntity();
		if (!pawn)
			return false;
		IEntity root = pawn.GetRootParent();
		if (!root || root == pawn)
			return false;
		return FindFirstLaserMarkingInHierarchy(root, maxDepth) != null;
	}

	//------------------------------------------------------------------------------------------------
	//! Local character is holding an HMD laser designator and is **actually using** it (LaserFixesAgain-style `IsZoomedForHMD` for handheld marking; legacy designation still uses gadget ADS).
	//! Accepts legacy `HMD_DesignationElementBaseComponent` gadgets or `HMD_LaserMarkingHandheldComponent` on the held item (spawned `Laser_Designation.et` + WCS bridge).
	static bool IsLocalHeldHmdLaserDesignatorUsingAds()
	{
		IEntity local = SCR_PlayerController.GetLocalMainEntity();
		if (!local)
			return false;
		SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(local);
		if (!gm)
			return false;
		SCR_GadgetComponent held = gm.GetHeldGadgetComponent();
		if (!held)
			return false;
		IEntity gadgetEnt = held.GetOwner();
		if (gadgetEnt && HMD_HandheldDesignatorOpticZoom.FindHandheldLaserMarkingOnGadget(gadgetEnt))
			return HMD_HandheldDesignatorOpticZoom.IsZoomedForHMD();
		return HMD_DesignationElementBaseComponent.Cast(held) != null && held.IsUsingADSControls();
	}

	//------------------------------------------------------------------------------------------------
	//! Same as IsLocalHeldHmdLaserDesignatorUsingAds, but the held gadget entity must match `gadgetEntity` (prefab gate).
	static bool IsLocalLookingThroughLaserDesignatorOnGadgetEntity(IEntity gadgetEntity)
	{
		if (!gadgetEntity)
			return false;
		if (!IsLocalHeldHmdLaserDesignatorUsingAds())
			return false;
		IEntity local = SCR_PlayerController.GetLocalMainEntity();
		if (!local)
			return false;
		SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(local);
		if (!gm)
			return false;
		SCR_GadgetComponent held = gm.GetHeldGadgetComponent();
		if (!held)
			return false;
		return held.GetOwner() == gadgetEntity;
	}

	//------------------------------------------------------------------------------------------------
	//! Local pawn mounted (vehicle / crew hierarchy): root parent is not the pawn itself.
	//! Prefer `GetLocalMainEntity` then `GetLocalControlledEntity` so this matches `SCR_AvailableActionsConditionData` character resolution used by control hints.
	static bool IsLocalMountedInVehicle()
	{
		IEntity pawn = SCR_PlayerController.GetLocalMainEntity();
		if (!pawn)
			pawn = SCR_PlayerController.GetLocalControlledEntity();
		if (!pawn)
			return false;
		return pawn.GetRootParent() != pawn;
	}

	//------------------------------------------------------------------------------------------------
	//! HUD layer toggles (markers / designations): vehicle seat allowed by `HMD_ElementVisualizationVehicleComponent` only.
	//! While **ADS through an HMD handheld designator**, numpad HUD actions are suppressed so only **T** and **`[` / `]`** apply.
	//! Character for the vehicle branch matches hint conditions (`GetLocalMainEntity` then `GetLocalControlledEntity`).
	static bool IsLocalEligibleForHmdHudToggleHints()
	{
		bool ads = IsLocalHeldHmdLaserDesignatorUsingAds();
		IEntity ch = SCR_PlayerController.GetLocalMainEntity();
		if (!ch)
			ch = SCR_PlayerController.GetLocalControlledEntity();
		bool vehGate = false;
		if (!ads && ch)
			vehGate = HMD_ElementVisualizationVehicleComponent.StaticIsLocalEligibleForVehicleHmdHud(ch);
		bool result = !ads && vehGate;
		string detail;
		if (ads)
			detail = "suppressed: handheld designator ADS / viewport";
		else if (!ch)
			detail = "no local pawn (main/controlled)";
		else if (!vehGate)
			detail = "vehicle HUD gate failed (seat / helmet / policy on vehicle root)";
		else
			detail = "ok: vehicle HUD eligible";
		HMD_HmdDebug.CliEligibilityKeyedChanged("HudLayerToggleHints", result, detail);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! True when `needle` matches `Replication.FindId` on `root` or any descendant that has an `RplComponent`.
	static bool EntitySubtreeContainsRplId(IEntity root, RplId needle)
	{
		if (!root || needle == RplId.Invalid())
			return false;
		RplComponent r = RplComponent.Cast(root.FindComponent(RplComponent));
		if (r && Replication.FindId(r) == needle)
			return true;
		IEntity ch = root.GetChildren();
		while (ch)
		{
			if (EntitySubtreeContainsRplId(ch, needle))
				return true;
			ch = ch.GetSibling();
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! True when `ent` is `potentialSubtreeRoot` or a descendant of it (walk `ent` ? parents).
	static bool EntityIsOrUnderRoot(IEntity ent, IEntity potentialSubtreeRoot)
	{
		if (!ent || !potentialSubtreeRoot)
			return false;
		IEntity w = ent;
		while (w)
		{
			if (w == potentialSubtreeRoot)
				return true;
			w = w.GetParent();
		}
		return false;
	}
}
