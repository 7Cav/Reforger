//------------------------------------------------------------------------------------------------
//! Client-side mirror subscription: eligibility gates send token deltas to the server via `HMD_PoolMirrorClientSinkComponent`.
//! Extend with `ClientSetMirrorToken` from gameplay code when additional contexts need the pool stream.
class HMD_PoolMirrorSubscription
{
	protected static ref map<string, bool> s_mLastSentTokenActive = new map<string, bool>();

	//------------------------------------------------------------------------------------------------
	static void ResetForNewPlaySession()
	{
		s_mLastSentTokenActive.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! Built-in gates: eligible vehicle HMD seat (`HMD_ElementVisualizationVehicleComponent`) and handheld designator viewport.
	static void PollBuiltinEligibilityAndSyncTokens()
	{
		if (!Replication.IsRunning() || !GetGame())
			return;
		if (!GetGame().GetPlayerController())
			return;

		IEntity ch = SCR_PlayerController.GetLocalMainEntity();
		if (!ch)
			ch = SCR_PlayerController.GetLocalControlledEntity();

		bool veh = false;
		if (ch)
			veh = HMD_ElementVisualizationVehicleComponent.StaticIsLocalEligibleForVehicleHmdHud(ch);

		bool handheld = HMD_HandheldDesignatorOpticZoom.IsZoomedForHMD();

		ClientSetMirrorToken("vehicle_hmd_seat", veh);
		ClientSetMirrorToken("handheld_designator_viewport", handheld);
	}

	//------------------------------------------------------------------------------------------------
	//! Mod / feature hooks: register an arbitrary token when your UI needs the pool mirror stream.
	static void ClientSetMirrorToken(string token, bool active)
	{
		if (!Replication.IsRunning() || !GetGame())
			return;
		if (!GetGame().GetPlayerController())
			return;
		if (!token || token.IsEmpty())
			return;

		bool prev = false;
		if (s_mLastSentTokenActive.Contains(token))
			prev = s_mLastSentTokenActive.Get(token);
		if (prev == active)
			return;

		s_mLastSentTokenActive.Set(token, active);

		HMD_PoolMirrorClientSinkComponent sink = HMD_PoolMirrorClientSinkComponent.GetLocalSink();
		if (!sink)
		{
			HMD_HmdDebug.CliPoolMirrorNet(string.Format("SubSend token=\"%1\" active=%2 sink=0 (add HMD_PoolMirrorClientSinkComponent to player controller)", token, active));
			return;
		}
		HMD_HmdDebug.CliPoolMirrorNet(string.Format("SubSend token=\"%1\" active=%2 sink=1", token, active));
		sink.ClientAskSetMirrorToken(token, active);
	}
}
