//------------------------------------------------------------------------------------------------
//! HMD diagnostic output is **disabled by default** except **`AREA_IFF_ATTACHABLE_BEACON_PLACE`** (IFF
//! attachable beacon placement / pool world position on authority). Enable other areas below only when
//! debugging those subsystems.
//!
//! Logging:
//! - `Print(..., LogLevel)` for Workbench **Scripts** console and engine log pipelines when enabled.
//! - **File append** to `$profile:HMD_Debug.log` (same `FileIO` pattern as other 7Cav scripts) so lines
//!   exist on disk without relying on `-logsDir` / host panel log capture. `$profile` is the game
//!   profile folder (client vs server each has its own profile on dedicated setups).
//! Default `WARNING` matches `ADS_DebugLog` for console filters; set `NORMAL` for quieter console output.
class HMD_HmdDebug
{
	//! When false, nothing is printed and nothing is written to `HMD_Debug.log` (all other toggles ignored).
	static const bool HMD_DEBUG_LOGGING_ENABLED = true;

	//! Routed to every `Print` from this module (explicit level improves capture vs single-arg `Print`).
	static const LogLevel HMD_SCRIPT_LOG_LEVEL = LogLevel.WARNING;

	//! When true, each emitted line is also appended to `$profile:HMD_Debug.log`.
	static const bool HMD_APPEND_TO_PROFILE_LOG_FILE = true;

	protected static const string HMD_LOG_FILE_PATH = "$profile:HMD_Debug.log";
	protected static bool s_bHmdLogFileSessionBannerWritten;

	static const int AREA_LASER_MARKING_CREATION = 1;
	static const int AREA_SERVER_STORAGE = 2;
	static const int AREA_CLIENT_POOL_MIRROR = 4;
	static const int AREA_CLIENT_ELIGIBILITY = 8;
	static const int AREA_LASER_REFERENCE_MODE = 16;
	static const int AREA_POOL_MIRROR_NET = 32;
	static const int AREA_IFF_ATTACHABLE_BEACON_PLACE = 64;

	static const bool HMD_DEBUG_LASER_MARKING_CREATION = false;
	static const bool HMD_DEBUG_SERVER_STORAGE = false;
	static const bool HMD_DEBUG_CLIENT_POOL_MIRROR = false;
	static const bool HMD_DEBUG_CLIENT_ELIGIBILITY = false;
	static const bool HMD_DEBUG_LASER_REFERENCE_MODE = false;
	static const bool HMD_DEBUG_POOL_MIRROR_NET = false;
	static const bool HMD_DEBUG_IFF_ATTACHABLE_BEACON_PLACE = true;

	//! Combined bitmask from the flags above.
	static const int ENABLED_AREAS = (HMD_DEBUG_LASER_MARKING_CREATION * AREA_LASER_MARKING_CREATION)
		| (HMD_DEBUG_SERVER_STORAGE * AREA_SERVER_STORAGE)
		| (HMD_DEBUG_CLIENT_POOL_MIRROR * AREA_CLIENT_POOL_MIRROR)
		| (HMD_DEBUG_CLIENT_ELIGIBILITY * AREA_CLIENT_ELIGIBILITY)
		| (HMD_DEBUG_LASER_REFERENCE_MODE * AREA_LASER_REFERENCE_MODE)
		| (HMD_DEBUG_POOL_MIRROR_NET * AREA_POOL_MIRROR_NET)
		| (HMD_DEBUG_IFF_ATTACHABLE_BEACON_PLACE * AREA_IFF_ATTACHABLE_BEACON_PLACE);

	static const int HINT_VEHICLE_LASER_CODE = 0;
	static const int HINT_HUD_LASER_MARKING_TOGGLE = 1;
	static const int HINT_HUD_LAYER_TOGGLE = 2;
	static const int HINT_HANDHELD_DESIGNATOR = 3;
	static const int HINT_LASER_REFERENCE_ENTER = 4;
	static const int HINT_LASER_REFERENCE_EXIT = 5;
	static const int HINT_LASER_REFERENCE_PLACE_RP = 6;
	static const int HINT_LASER_REFERENCE_PLACE_WP = 7;
	static const int HINT_LASER_REFERENCE_DELETE = 8;
	protected static const int HINT_COUNT = 9;

	protected static ref map<string, int> s_mEligibilityKeyedState;
	protected static ref array<int> s_aHintLastState;
	protected static int s_iLastSrvTraceFailLogMs;
	protected static int s_iLastCliHudSummaryMs;
	protected static int s_iLastCliLaserRefHudMs;
	protected static int s_iSrvPoolMirrorNetPosFanoutLastMs;
	protected static int s_iCliPoolMirrorNetPosRxLastMs;

	//------------------------------------------------------------------------------------------------
	static void ResetForNewPlaySession()
	{
		if (s_mEligibilityKeyedState)
			s_mEligibilityKeyedState.Clear();
		if (!s_aHintLastState)
			s_aHintLastState = new array<int>();
		s_aHintLastState.Clear();
		for (int i = 0; i < HINT_COUNT; i++)
			s_aHintLastState.Insert(-1);
		s_iLastSrvTraceFailLogMs = 0;
		s_iLastCliHudSummaryMs = 0;
		s_iLastCliLaserRefHudMs = 0;
		s_iSrvPoolMirrorNetPosFanoutLastMs = 0;
		s_iCliPoolMirrorNetPosRxLastMs = 0;
		s_bHmdLogFileSessionBannerWritten = false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool AreaEnabled(int area)
	{
		if (!HMD_DEBUG_LOGGING_ENABLED)
			return false;
		return (ENABLED_AREAS & area) != 0;
	}

	//------------------------------------------------------------------------------------------------
	//! True when this process has a local `PlayerController` (dedicated client, listen host, or SP).
	protected static bool PoolMirrorNetLocalControllerReady()
	{
		if (!Replication.IsRunning() || !GetGame())
			return false;
		if (!GetGame().GetPlayerController())
			return false;
		if (Replication.IsClient())
			return true;
		return Replication.IsServer();
	}

	//------------------------------------------------------------------------------------------------
	protected static void EmitLine(string line)
	{
		if (!HMD_DEBUG_LOGGING_ENABLED)
			return;
		Print(line, HMD_SCRIPT_LOG_LEVEL);
		if (!HMD_APPEND_TO_PROFILE_LOG_FILE)
			return;
		FileHandle f = FileIO.OpenFile(HMD_LOG_FILE_PATH, FileMode.APPEND);
		if (!f || !f.IsOpen())
			return;
		if (!s_bHmdLogFileSessionBannerWritten)
		{
			s_bHmdLogFileSessionBannerWritten = true;
			int srv = 0;
			if (Replication.IsServer())
				srv = 1;
			int cli = 0;
			if (Replication.IsClient())
				cli = 1;
			string banner = string.Format("==== HMD_Debug file log session tick=%1 IsServer=%2 IsClient=%3 ====\n", System.GetTickCount(), srv, cli);
			f.Write(banner);
		}
		f.Write(string.Format("%1\n", line));
		f.Close();
	}

	//------------------------------------------------------------------------------------------------
	static void SrvLaserMarking(string msg)
	{
		if (!AreaEnabled(AREA_LASER_MARKING_CREATION) || !Replication.IsServer())
			return;
		EmitLine(string.Format("[HMD][Srv][LaserMarking] %1", msg));
	}

	//------------------------------------------------------------------------------------------------
	static void SrvLaserRef(string msg)
	{
		if (!AreaEnabled(AREA_LASER_REFERENCE_MODE) || !Replication.IsServer())
			return;
		EmitLine(string.Format("[HMD][Srv][LaserRef] %1", msg));
	}

	//------------------------------------------------------------------------------------------------
	static void CliLaserRef(string msg)
	{
		if (!AreaEnabled(AREA_LASER_REFERENCE_MODE) || !Replication.IsClient())
			return;
		EmitLine(string.Format("[HMD][Cli][LaserRef] %1", msg));
	}

	//------------------------------------------------------------------------------------------------
	static void CliLaserRefHudThrottled(string msg, int throttleMs)
	{
		if (!AreaEnabled(AREA_LASER_REFERENCE_MODE) || !Replication.IsClient())
			return;
		int now = System.GetTickCount();
		if (s_iLastCliLaserRefHudMs != 0 && now - s_iLastCliLaserRefHudMs < throttleMs)
			return;
		s_iLastCliLaserRefHudMs = now;
		EmitLine(string.Format("[HMD][Cli][LaserRef][HudBridge] %1", msg));
	}

	//------------------------------------------------------------------------------------------------
	//! Throttled trace / LOS failures (avoids spam while marking is on with no valid hit).
	static void SrvLaserMarkingThrottled(string msg, int throttleMs)
	{
		if (!AreaEnabled(AREA_LASER_MARKING_CREATION) || !Replication.IsServer())
			return;
		int now = System.GetTickCount();
		if (now - s_iLastSrvTraceFailLogMs < throttleMs)
			return;
		s_iLastSrvTraceFailLogMs = now;
		EmitLine(string.Format("[HMD][Srv][LaserMarking] %1", msg));
	}

	//------------------------------------------------------------------------------------------------
	static void SrvStorage(string msg)
	{
		if (!AreaEnabled(AREA_SERVER_STORAGE) || !Replication.IsServer())
			return;
		EmitLine(string.Format("[HMD][Srv][Storage] %1", msg));
	}

	//------------------------------------------------------------------------------------------------
	static void CliPoolMirror(string msg)
	{
		if (!AreaEnabled(AREA_CLIENT_POOL_MIRROR) || !Replication.IsClient())
			return;
		EmitLine(string.Format("[HMD][Cli][PoolMirror] %1", msg));
	}

	//------------------------------------------------------------------------------------------------
	//! Client: mirror **subscription** (token deltas, sink send path).
	static void CliPoolMirrorNet(string msg)
	{
		if (!AreaEnabled(AREA_POOL_MIRROR_NET) || !PoolMirrorNetLocalControllerReady())
			return;
		EmitLine(string.Format("[HMD][Cli][PoolMirrorNet] %1", msg));
	}

	//------------------------------------------------------------------------------------------------
	//! Server: mirror **subscriptions** and **fan-out** (who gets owner RPCs).
	static void SrvPoolMirrorNet(string msg)
	{
		if (!AreaEnabled(AREA_POOL_MIRROR_NET) || !Replication.IsServer())
			return;
		EmitLine(string.Format("[HMD][Srv][PoolMirrorNet] %1", msg));
	}

	//------------------------------------------------------------------------------------------------
	//! Server: throttled log for high-rate position **fan-out** to subscribers.
	static void SrvPoolMirrorNetPosFanoutThrottled(string msg, int throttleMs)
	{
		if (!AreaEnabled(AREA_POOL_MIRROR_NET) || !Replication.IsServer())
			return;
		int now = System.GetTickCount();
		if (s_iSrvPoolMirrorNetPosFanoutLastMs != 0 && now - s_iSrvPoolMirrorNetPosFanoutLastMs < throttleMs)
			return;
		s_iSrvPoolMirrorNetPosFanoutLastMs = now;
		EmitLine(string.Format("[HMD][Srv][PoolMirrorNet] %1", msg));
	}

	//------------------------------------------------------------------------------------------------
	//! Client: throttled log for owner-Rx **position batches** on the pool mirror sink.
	static void CliPoolMirrorNetPosRxThrottled(string msg, int throttleMs)
	{
		if (!AreaEnabled(AREA_POOL_MIRROR_NET) || !PoolMirrorNetLocalControllerReady())
			return;
		int now = System.GetTickCount();
		if (s_iCliPoolMirrorNetPosRxLastMs != 0 && now - s_iCliPoolMirrorNetPosRxLastMs < throttleMs)
			return;
		s_iCliPoolMirrorNetPosRxLastMs = now;
		EmitLine(string.Format("[HMD][Cli][PoolMirrorNet] %1", msg));
	}

	//------------------------------------------------------------------------------------------------
	static void CliEligibility(string msg)
	{
		if (!AreaEnabled(AREA_CLIENT_ELIGIBILITY) || !Replication.IsClient())
			return;
		EmitLine(string.Format("[HMD][Cli][Eligibility] %1", msg));
	}

	//------------------------------------------------------------------------------------------------
	//! Logs only when the boolean result for `key` changes (first sample sets baseline without printing).
	static void CliEligibilityKeyedChanged(string key, bool nowOk, string detail)
	{
		if (!AreaEnabled(AREA_CLIENT_ELIGIBILITY) || !Replication.IsClient())
			return;
		if (!s_mEligibilityKeyedState)
			s_mEligibilityKeyedState = new map<string, int>();
		int asInt = 0;
		if (nowOk)
			asInt = 1;
		int prev = -1;
		if (s_mEligibilityKeyedState.Contains(key))
			prev = s_mEligibilityKeyedState.Get(key);
		if (prev < 0)
		{
			s_mEligibilityKeyedState.Set(key, asInt);
			return;
		}
		if (prev == asInt)
			return;
		s_mEligibilityKeyedState.Set(key, asInt);
		EmitLine(string.Format("[HMD][Cli][Eligibility][%1] now=%2 | %3", key, nowOk, detail));
	}

	//------------------------------------------------------------------------------------------------
	//! Control-hint rows: logs only when availability changes (first call sets baseline).
	static void CliEligibilityHintChanged(int hintIndex, bool nowOk, string detail)
	{
		if (!AreaEnabled(AREA_CLIENT_ELIGIBILITY) || !Replication.IsClient())
			return;
		if (hintIndex < 0 || hintIndex >= HINT_COUNT)
			return;
		if (!s_aHintLastState || s_aHintLastState.Count() != HINT_COUNT)
		{
			if (!s_aHintLastState)
				s_aHintLastState = new array<int>();
			s_aHintLastState.Clear();
			for (int i = 0; i < HINT_COUNT; i++)
				s_aHintLastState.Insert(-1);
		}
		int asInt = 0;
		if (nowOk)
			asInt = 1;
		if (s_aHintLastState[hintIndex] < 0)
		{
			s_aHintLastState[hintIndex] = asInt;
			return;
		}
		if (s_aHintLastState[hintIndex] == asInt)
			return;
		s_aHintLastState[hintIndex] = asInt;
		EmitLine(string.Format("[HMD][Cli][Eligibility][Hint:%1] now=%2 | %3", hintIndex, nowOk, detail));
	}

	//------------------------------------------------------------------------------------------------
	static void CliHudDrawSummaryThrottled(string msg, int throttleMs)
	{
		if (!AreaEnabled(AREA_CLIENT_ELIGIBILITY) || !Replication.IsClient())
			return;
		int now = System.GetTickCount();
		if (s_iLastCliHudSummaryMs != 0 && now - s_iLastCliHudSummaryMs < throttleMs)
			return;
		s_iLastCliHudSummaryMs = now;
		EmitLine(string.Format("[HMD][Cli][Eligibility][HudBridge] %1", msg));
	}

	//------------------------------------------------------------------------------------------------
	//! Server / listen authority: IFF attachable beacon placement and pool world position.
	static void SrvIffBeaconPlace(string msg)
	{
		if (!AreaEnabled(AREA_IFF_ATTACHABLE_BEACON_PLACE) || !Replication.IsServer())
			return;
		EmitLine(string.Format("[HMD][Srv][IffBeaconPlace] %1", msg));
	}

	//------------------------------------------------------------------------------------------------
	//! Client: deferred placement notify, Rpc send, local origin samples.
	static void CliIffBeaconPlace(string msg)
	{
		if (!AreaEnabled(AREA_IFF_ATTACHABLE_BEACON_PLACE) || !Replication.IsClient())
			return;
		EmitLine(string.Format("[HMD][Cli][IffBeaconPlace] %1", msg));
	}
}
