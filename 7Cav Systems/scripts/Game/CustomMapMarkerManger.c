modded class SCR_MapMarkerManagerComponent
{
	override void OnAskAddStaticMarker(SCR_MapMarkerBase markerData)
	{
		// Force all player-placed map markers (including Map Drawing lines) to be visible server-wide
		markerData.SetMarkerFactionFlags(0);
		super.OnAskAddStaticMarker(markerData);
	}
}
