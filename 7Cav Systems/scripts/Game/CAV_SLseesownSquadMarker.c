//------------------------------------------------------------------------------------------------
//! Element leader map markers: every friendly leader's marker is visible to the whole side,
//! including the leader's own marker.
//!
//! Vanilla already does the hard part. SCR_MapMarkerEntrySquadLeader creates one marker per
//! playable group, keeps its position live, carries the group flag and callsign, and replicates
//! the owning player id. Only the client-side visibility rule is wrong for us, so that rule is
//! the only thing overridden here.
//!
//! Vanilla's rule, in UpdateLocalVisibility:
//!
//!   1. Your own marker is always hidden.
//!   2. If you are in no group, you see nothing at all.
//!   3. Otherwise leaders and members see other leaders per two prefab flags.
//!
//! Rules 1 and 2 are the problems. A leader with no marker of their own has no position
//! reference on the map, and rule 2 blanks the map for anyone who has not joined a group, which
//! on a sandbox server is a large share of the players at any moment. Neither prefab flag
//! reaches either case, which is why this is a script override rather than a prefab edit.
//!
//! Our rule: a marker is visible when its owner is in the local player's faction, own marker
//! included. Both factions resolve client-side from the replicated player id, so no server round
//! trip is involved.
//!
//! WHY THIS OVERRIDES UpdateLocalVisibility AND NOT OnPlayerIdUpdate. The previous version of
//! this file overrode OnPlayerIdUpdate and called SetLocalVisible(true) directly, which does not
//! hold. OnPlayerIdUpdate is not the visibility rule, it is one of four triggers that REQUEST a
//! visibility recalculation by raising m_bDoLocalVisibilityUpdate for OnUpdate to service. The
//! other three are OnPlayerAdded, OnPlayerLeaderChanged and OnFactionCommanderChanged. So the
//! forced-visible state survived only until the next time anyone joined a group, a group changed
//! leader, or the faction commander changed; UpdateLocalVisibility then ran untouched and hid the
//! leader's own marker again by vanilla rule 1. Overriding the rule itself is stable under all
//! four triggers. It also stops the old version from setting visibility before the faction check
//! in OnCreateMarker has had a say.
//!
//! Deliberately NOT touched: SCR_MapMarkerEntrySquadLeader, the config entry. It is instantiated
//! by name from a config container, and modding such a class unregisters it - the entry is then
//! dropped and no leader markers exist at all.
modded class SCR_MapMarkerSquadLeader
{
	//------------------------------------------------------------------------------------------------
	override void UpdateLocalVisibility()
	{
		m_bDoLocalVisibilityUpdate = false;

		PlayerController pController = GetGame().GetPlayerController();
		if (!pController)
			return;

		// Own marker deliberately NOT hidden here, unlike vanilla: a leader needs a position
		// reference of their own on the map.

		Faction localFaction = SCR_FactionManager.SGetPlayerFaction(pController.GetPlayerId());
		Faction markerFaction = SCR_FactionManager.SGetPlayerFaction(m_PlayerID);
		if (!localFaction || !markerFaction || localFaction != markerFaction)
		{
			SetLocalVisible(false);
			return;
		}

		SetLocalVisible(true);
	}
}
