//------------------------------------------------------------------------------------------------
//! Collects all HMD_ElementVisualizationBaseComponent instances for OR-gated HUD draw policy.
class HMD_ElementVisualizationRegistry
{
	protected static ref array<HMD_ElementVisualizationBaseComponent> s_aViz = {};

	//------------------------------------------------------------------------------------------------
	//! Drops dead refs when a play session boundary fires (Workbench replay without script reload).
	//! Do not `Clear()` the whole list: editor / Workbench entities often keep the same component instances
	//! without a second `OnPostInit`, so a full clear orphans every gate while hints still find the component on the entity tree.
	static void ResetForNewPlaySession()
	{
		PruneInvalidVisualizationEntries();
	}

	//------------------------------------------------------------------------------------------------
	protected static int PruneInvalidVisualizationEntries()
	{
		int removed = 0;
		if (!s_aViz)
			return 0;
		for (int i = s_aViz.Count() - 1; i >= 0; i--)
		{
			HMD_ElementVisualizationBaseComponent c = s_aViz[i];
			if (!c || !c.GetOwner())
			{
				s_aViz.RemoveOrdered(i);
				removed++;
			}
		}
		return removed;
	}

	//------------------------------------------------------------------------------------------------
	static void Register(HMD_ElementVisualizationBaseComponent c)
	{
		if (!c)
			return;
		if (s_aViz.Find(c) >= 0)
			return;
		s_aViz.Insert(c);
	}

	//------------------------------------------------------------------------------------------------
	static void Unregister(HMD_ElementVisualizationBaseComponent c)
	{
		int i = s_aViz.Find(c);
		if (i >= 0)
			s_aViz.RemoveOrdered(i);
	}

	//------------------------------------------------------------------------------------------------
	static bool AnyGateActive()
	{
		foreach (HMD_ElementVisualizationBaseComponent c : s_aViz)
		{
			if (c && c.HmdIsVisualizationGateActive())
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	static float GetEffectiveMaxViewDistanceM()
	{
		float best = 999999.0;
		bool any = false;
		foreach (HMD_ElementVisualizationBaseComponent c : s_aViz)
		{
			if (!c || !c.HmdIsVisualizationGateActive())
				continue;
			float m = c.HmdGetMaxViewDistanceM();
			if (!any || m < best)
				best = m;
			any = true;
		}
		if (!any)
			return 2000.0;
		return best;
	}

	//------------------------------------------------------------------------------------------------
	//! When the local HMD gate is a turret visualization, returns that turret entity's `RplId` for pool row scoping (WP/RP).
	static RplId GetActiveTurretInformationalScopeParent0()
	{
		foreach (HMD_ElementVisualizationBaseComponent c : s_aViz)
		{
			if (!c || !c.HmdIsVisualizationGateActive())
				continue;
			HMD_ElementVisualizationTurretComponent t = HMD_ElementVisualizationTurretComponent.Cast(c);
			if (!t)
				continue;
			IEntity te = t.GetOwner();
			if (!te)
				continue;
			RplComponent rpl = RplComponent.Cast(te.FindComponent(RplComponent));
			if (!rpl)
				continue;
			return Replication.FindId(rpl);
		}
		return RplId.Invalid();
	}
}
