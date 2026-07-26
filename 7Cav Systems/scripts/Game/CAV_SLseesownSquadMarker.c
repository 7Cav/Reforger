modded class SCR_MapMarkerSquadLeader
{
	override void OnPlayerIdUpdate()
	{
		PlayerController pController = GetGame().GetPlayerController();
		if (!pController)
			return;
		
		// Unlike vanilla, squad leaders also see their own squad marker.
		SetLocalVisible(true);
	}
}
