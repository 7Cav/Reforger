//------------------------------------------------------------------------------------------------
//! Zoom / ADS for HMD handheld laser marking: held gadget must carry `HMD_LaserMarkingHandheldComponent` in its subtree.
//! Mirrors LaserFixesAgain `HMD_HandheldOpticZoom` (binocular gadgets use `IsZoomedView` only; optic-only uses sights ADS).
class HMD_HandheldDesignatorOpticZoom
{
	//------------------------------------------------------------------------------------------------
	static HMD_LaserMarkingHandheldComponent FindHandheldLaserMarkingOnGadget(IEntity root)
	{
		if (!root)
			return null;
		HMD_LaserMarkingHandheldComponent h = HMD_LaserMarkingHandheldComponent.Cast(root.FindComponent(HMD_LaserMarkingHandheldComponent));
		if (h)
			return h;
		IEntity child = root.GetChildren();
		while (child)
		{
			HMD_LaserMarkingHandheldComponent f = FindHandheldLaserMarkingOnGadget(child);
			if (f)
				return f;
			child = child.GetSibling();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_2DOpticsComponent Find2DOpticsInHierarchy(IEntity root)
	{
		if (!root)
			return null;
		SCR_2DOpticsComponent o = SCR_2DOpticsComponent.Cast(root.FindComponent(SCR_2DOpticsComponent));
		if (o)
			return o;
		IEntity child = root.GetChildren();
		while (child)
		{
			SCR_2DOpticsComponent f = Find2DOpticsInHierarchy(child);
			if (f)
				return f;
			child = child.GetSibling();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_BinocularsComponent FindBinocularsInHierarchy(IEntity root)
	{
		if (!root)
			return null;
		SCR_BinocularsComponent b = SCR_BinocularsComponent.Cast(root.FindComponent(SCR_BinocularsComponent));
		if (b)
			return b;
		IEntity child = root.GetChildren();
		while (child)
		{
			SCR_BinocularsComponent f = FindBinocularsInHierarchy(child);
			if (f)
				return f;
			child = child.GetSibling();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static BaseSightsComponent FindBaseSightsInHierarchy(IEntity root)
	{
		if (!root)
			return null;
		BaseSightsComponent s = BaseSightsComponent.Cast(root.FindComponent(BaseSightsComponent));
		if (s)
			return s;
		IEntity child = root.GetChildren();
		while (child)
		{
			BaseSightsComponent f = FindBaseSightsInHierarchy(child);
			if (f)
				return f;
			child = child.GetSibling();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	static IEntity ResolveLocalCharacterEntity()
	{
		IEntity main = SCR_PlayerController.GetLocalMainEntity();
		if (main)
			return main;
		return SCR_PlayerController.GetLocalControlledEntity();
	}

	//------------------------------------------------------------------------------------------------
	//! True when looking through handheld optics for an HMD handheld designator (matches LaserFixesAgain `IsZoomedForHMD`).
	static bool IsZoomedForHMD()
	{
		IEntity localChar = ResolveLocalCharacterEntity();
		if (!localChar)
			return false;

		SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(localChar);
		if (!gm)
			return false;

		IEntity held = gm.GetHeldGadget();
		if (!held)
			return false;

		if (!FindHandheldLaserMarkingOnGadget(held))
			return false;

		if (FindBinocularsInHierarchy(held))
			return SCR_BinocularsComponent.IsZoomedView();

		if (SCR_BinocularsComponent.IsZoomedView())
			return true;

		SCR_2DOpticsComponent optics = Find2DOpticsInHierarchy(held);
		if (optics && optics.IsSightADSActive())
			return true;

		BaseSightsComponent sights = FindBaseSightsInHierarchy(held);
		if (sights && sights.IsSightADSActive())
			return true;

		return false;
	}
}
