modded class SCR_MapMarkerManagerComponent
{
	override void OnAskAddStaticMarker(SCR_MapMarkerBase markerData)
	{
		// Force all player-placed map markers (including Map Drawing lines) to be visible server-wide.
		// Server-authored markers keep their faction flags: SCR_AIEnemyMarkingSystem stores the same
		// SCR_MapMarkerBase instance it passes here and later matches on GetMarkerFactionFlags() to
		// merge/replace its own contact reports. Wiping the flags on those breaks that de-duplication
		// (markers accumulate unbounded) and broadcasts one faction's AI contact reports to the other.
		// InsertStaticMarker sets owner -1 for server markers; SCR_MapMarkerSyncComponent sets a real
		// player id for player-placed ones. Both happen before this call.
		if (markerData.GetMarkerOwnerID() > -1)
			markerData.SetMarkerFactionFlags(0);

		super.OnAskAddStaticMarker(markerData);
	}
}
