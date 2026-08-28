// Vanilla only reveals a placed marker's custom text while the cursor is over the marker, and
// there is no config attribute or user setting for it. SCR_MapMarkerWidgetComponent.SetTextVisible()
// records the wanted state in m_bShowText but then hides the text widget unconditionally; only
// OnMouseEnter puts it back, and OnMouseLeave takes it away again.
//
// The gate is applied in the widget component rather than in the marker entry configs because the
// entry configs are [BaseContainerProps()] classes built from a .conf, and both of the types we
// care about - PLACED_CUSTOM (SCR_MapMarkerEntryPlaced) and PLACED_MILITARY
// (SCR_MapMarkerEntryMilitary) - route through this one component anyway. Both feed the same
// m_wMarkerText via SetText(marker.GetCustomText()).
//
// Deliberately limited to those two player-placed types. The same SetTextVisible() also drives the
// squad leader and squad member markers, and pinning those labels open would bury the map under
// squad and player names.

modded class SCR_MapMarkerWidgetComponent
{
	//------------------------------------------------------------------------------------------------
	//! True for the marker types a player places by hand and types a label into.
	protected bool CAV_IsPlayerPlacedMarker()
	{
		if (!m_MarkerObject)
			return false;

		SCR_EMapMarkerType type = m_MarkerObject.GetType();

		return type == SCR_EMapMarkerType.PLACED_CUSTOM || type == SCR_EMapMarkerType.PLACED_MILITARY;
	}

	//------------------------------------------------------------------------------------------------
	override void SetTextVisible(bool state)
	{
		super.SetTextVisible(state);

		if (!m_wMarkerText || !CAV_IsPlayerPlacedMarker())
			return;

		// Ignore the requested state - callers pass false for every map layer past 1. m_bShowText is
		// pinned true as well so the OnMouseEnter/OnMouseLeave branches stay in agreement.
		m_bShowText = true;
		m_wMarkerText.SetVisible(true);
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		bool result = super.OnMouseLeave(w, enterW, x, y);

		// super restores the real map layer and then re-hides the text on the way out.
		if (m_wMarkerText && CAV_IsPlayerPlacedMarker())
			m_wMarkerText.SetVisible(true);

		return result;
	}
}
