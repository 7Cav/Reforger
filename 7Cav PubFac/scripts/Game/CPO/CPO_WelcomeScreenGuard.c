//------------------------------------------------------------------------------------------------
//! Keeps the deploy screen's faction widget list the same length as its faction list.
//!
//! SCR_WelcomeScreenFactionContent holds two parallel arrays and one of them is not built for every
//! faction:
//!
//!     protected ref array<Widget> m_aFactionWidgets;                      // one entry per shown faction
//!     protected ref SCR_SortedArray<SCR_Faction> m_SortedFactions;        // one entry per faction
//!
//! AddFactionWidget walks the sorted list and calls FillFactionWidget for the first six entries,
//! and FillFactionWidget has four return paths, only two of which insert a widget:
//!
//!     line 1090   never-show override, or the faction is not playable yet   -> returns, no widget
//!     line 1094   the widget could not be created                           -> returns, no widget
//!     line 1117   faction key is not the configured US/USSR/FIA key          -> returns, widget added
//!     line 1137   the widget has no player-count text                        -> returns, widget added
//!
//! Any faction that takes one of the first two paths therefore leaves the widget array SHORTER than
//! the sorted list, and every faction after it sits at a widget index lower than its sorted index.
//! The playability handler then indexes one array with the other array's number:
//!
//!     Widget factionWidget = m_aFactionWidgets[m_SortedFactions.Find(factionScripted)];   // line 1197
//!
//! A civilian or FIA faction that is not playable is skipped on a normal boot, so the shift is the
//! normal case rather than an edge case, and the visible symptom is exact: the faction has a button
//! on the deploy screen, and clicking it (which changes playability and fires the handler) trips
//!
//!     Reason: Index out of bounds.
//!     Class: 'SCR_WelcomeScreenFactionContent'   Function: 'Get'
//!     scripts/Game/GameMode/Respawn/SCR_WelcomeScreenComponent.c:1197 UpdateFactionPlayability
//!
//! This keeps one widget-array entry per filled faction, using a null placeholder where vanilla
//! skipped one, and pads the tail for the factions past the six-widget cap that never reach
//! FillFactionWidget at all. Both handlers null-check the widget they read, so a null entry is
//! inert, and nothing about the screen changes: a placeholder renders nothing, exactly as before.
//! The point is that the faction that IS shown now updates the right widget instead of the wrong
//! one or none at all.
//!
//! The guard on the handler itself stays as the second layer: with the arrays level it never fires,
//! so if it ever logs, something else has changed the array lengths and the log says so.
modded class SCR_WelcomeScreenFactionContent
{
	//------------------------------------------------------------------------------------------------
	override protected void FillFactionWidget(notnull SCR_Faction faction, notnull Widget content, Color color)
	{
		int before = m_aFactionWidgets.Count();
		super.FillFactionWidget(faction, content, color);

		if (m_aFactionWidgets.Count() == before)
			m_aFactionWidgets.Insert(null);
	}

	//------------------------------------------------------------------------------------------------
	override protected void AddFactionWidget()
	{
		super.AddFactionWidget();

		while (m_aFactionWidgets.Count() < m_SortedFactions.Count())
			m_aFactionWidgets.Insert(null);
	}

	//------------------------------------------------------------------------------------------------
	override protected void UpdateFactionPlayability(Faction faction, bool playable)
	{
		SCR_Faction factionScripted = SCR_Faction.Cast(faction);
		if (!factionScripted)
			return;

		int index = m_SortedFactions.Find(factionScripted);
		if (index < 0 || index >= m_aFactionWidgets.Count())
		{
			Print("[" + CPO_FactionsInfo.NAME + "] deploy screen: " + factionScripted.GetFactionKey() + " has no faction widget, playability update skipped", LogLevel.NORMAL);
			return;
		}

		super.UpdateFactionPlayability(faction, playable);
	}
}
