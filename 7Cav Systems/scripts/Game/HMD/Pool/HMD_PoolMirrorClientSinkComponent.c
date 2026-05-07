//------------------------------------------------------------------------------------------------
//! Per-player Rpc transport for HMD pool mirror updates (`RplRcver.Owner`), replacing broadcast pool RPCs.
//! Lives on `SCR_PlayerController`; server calls `Authority_*` to push rows only to subscribed clients.
[ComponentEditorProps(category: "HMD", description: "Owner-targeted pool mirror RPC sink on player controller.")]
class HMD_PoolMirrorClientSinkComponentClass : ScriptComponentClass
{
}

class HMD_PoolMirrorClientSinkComponent : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	static HMD_PoolMirrorClientSinkComponent FindSinkForPlayerId(PlayerManager pm, int playerId)
	{
		if (!pm)
			return null;
		PlayerController pcb = pm.GetPlayerController(playerId);
		SCR_PlayerController spc = SCR_PlayerController.Cast(pcb);
		if (!spc)
			return null;
		return HMD_PoolMirrorClientSinkComponent.Cast(spc.FindComponent(HMD_PoolMirrorClientSinkComponent));
	}

	//------------------------------------------------------------------------------------------------
	static HMD_PoolMirrorClientSinkComponent GetLocalSink()
	{
		if (!GetGame())
			return null;
		PlayerController pl = GetGame().GetPlayerController();
		SCR_PlayerController spc = SCR_PlayerController.Cast(pl);
		if (!spc)
			return null;
		return HMD_PoolMirrorClientSinkComponent.Cast(spc.FindComponent(HMD_PoolMirrorClientSinkComponent));
	}

	//------------------------------------------------------------------------------------------------
	void ClientAskSetMirrorToken(string token, bool active)
	{
		if (!Replication.IsRunning())
			return;
		if (Replication.IsServer() && !Replication.IsClient())
		{
			HMD_HmdDebug.CliPoolMirrorNet(string.Format("SubTransport token=\"%1\" active=%2 path=DirectServerApply", token, active));
			ServerApplyMirrorTokenForOwnerController(token, active);
			return;
		}
		if (!Replication.IsClient())
			return;
		HMD_HmdDebug.CliPoolMirrorNet(string.Format("SubTransport token=\"%1\" active=%2 path=RpcAsk_Server", token, active));
		Rpc(RpcAsk_SetMirrorToken, token, active);
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerApplyMirrorTokenForOwnerController(string token, bool active)
	{
		if (!Replication.IsServer())
			return;
		SCR_PlayerController spc = SCR_PlayerController.Cast(GetOwner());
		if (!spc)
			return;
		int pid = spc.GetPlayerId();
		if (pid < 0)
			return;
		HMD_HmdDebug.SrvPoolMirrorNet(string.Format("SubFromSink playerId=%1 token=\"%2\" active=%3", pid, token, active));
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (pool)
			pool.ServerSetClientMirrorToken(pid, token, active);
		else
			HMD_HmdDebug.SrvPoolMirrorNet("SubFromSink pool=null (game mode pool not spawned yet?)");
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SetMirrorToken(string token, bool active)
	{
		ServerApplyMirrorTokenForOwnerController(token, active);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: push meta to this controller's owning client only.
	void Authority_PoolMirror_MetaAdd(int id, EHmdElementKind kind, int classType, string text, int code, RplId parent0, RplId parent1, bool showEvenWhenDesignationHudOff)
	{
		if (!Replication.IsServer())
			return;
		Rpc(RpcDo_PoolMirror_MetaAdd, id, kind, classType, text, code, parent0, parent1, showEvenWhenDesignationHudOff);
		if (RplSession.Mode() != RplMode.Dedicated && Replication.IsClient())
		{
			HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
			if (pool)
				pool.ClientApplyMirrorMetaAdd(id, kind, classType, text, code, parent0, parent1, showEvenWhenDesignationHudOff);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Authority_PoolMirror_MetaHudColor(int id, int hudColorArgb)
	{
		if (!Replication.IsServer())
			return;
		Rpc(RpcDo_PoolMirror_MetaHudColor, id, hudColorArgb);
		if (RplSession.Mode() != RplMode.Dedicated && Replication.IsClient())
		{
			HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
			if (pool)
				pool.ClientApplyMirrorMetaHudColor(id, hudColorArgb);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Authority_PoolMirror_MetaRemove(int id)
	{
		if (!Replication.IsServer())
			return;
		Rpc(RpcDo_PoolMirror_MetaRemove, id);
		if (RplSession.Mode() != RplMode.Dedicated && Replication.IsClient())
		{
			HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
			if (pool)
				pool.ClientApplyMirrorMetaRemove(id);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Authority_PoolMirror_MetaCode(int id, int newCode)
	{
		if (!Replication.IsServer())
			return;
		Rpc(RpcDo_PoolMirror_MetaCode, id, newCode);
		if (RplSession.Mode() != RplMode.Dedicated && Replication.IsClient())
		{
			HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
			if (pool)
				pool.ClientApplyMirrorMetaCode(id, newCode);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Authority_PoolMirror_MetaText(int id, string text)
	{
		if (!Replication.IsServer())
			return;
		Rpc(RpcDo_PoolMirror_MetaText, id, text);
		if (RplSession.Mode() != RplMode.Dedicated && Replication.IsClient())
		{
			HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
			if (pool)
				pool.ClientApplyMirrorMetaText(id, text);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Authority_PoolMirror_PositionBatch(array<int> ids, array<vector> positions)
	{
		if (!Replication.IsServer())
			return;
		Rpc(RpcDo_PoolMirror_PositionBatch, ids, positions);
		if (RplSession.Mode() != RplMode.Dedicated && Replication.IsClient())
		{
			HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
			if (pool)
				pool.ClientApplyMirrorPositionBatch(ids, positions);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Authority_PoolMirror_ClearAll()
	{
		if (!Replication.IsServer())
			return;
		Rpc(RpcDo_PoolMirror_ClearAll);
		if (RplSession.Mode() != RplMode.Dedicated && Replication.IsClient())
		{
			HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
			if (pool)
				pool.ClientApplyMirrorClearAll();
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_PoolMirror_MetaAdd(int id, EHmdElementKind kind, int classType, string text, int code, RplId parent0, RplId parent1, bool showEvenWhenDesignationHudOff)
	{
		HMD_HmdDebug.CliPoolMirrorNet(string.Format("SinkRx MetaAdd id=%1 kind=%2 class=%3 code=%4", id, typename.EnumToString(EHmdElementKind, kind), classType, code));
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (pool)
			pool.ClientApplyMirrorMetaAdd(id, kind, classType, text, code, parent0, parent1, showEvenWhenDesignationHudOff);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_PoolMirror_MetaHudColor(int id, int hudColorArgb)
	{
		HMD_HmdDebug.CliPoolMirrorNet(string.Format("SinkRx MetaHudColor id=%1 argb=%2", id, hudColorArgb));
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (pool)
			pool.ClientApplyMirrorMetaHudColor(id, hudColorArgb);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_PoolMirror_MetaRemove(int id)
	{
		HMD_HmdDebug.CliPoolMirrorNet(string.Format("SinkRx MetaRemove id=%1", id));
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (pool)
			pool.ClientApplyMirrorMetaRemove(id);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_PoolMirror_MetaCode(int id, int newCode)
	{
		HMD_HmdDebug.CliPoolMirrorNet(string.Format("SinkRx MetaCode id=%1 code=%2", id, newCode));
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (pool)
			pool.ClientApplyMirrorMetaCode(id, newCode);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_PoolMirror_MetaText(int id, string text)
	{
		HMD_HmdDebug.CliPoolMirrorNet(string.Format("SinkRx MetaText id=%1 text=\"%2\"", id, text));
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (pool)
			pool.ClientApplyMirrorMetaText(id, text);
	}

	//------------------------------------------------------------------------------------------------
	//! Reliable: avoids dropped static IFF rows (no follow-up delta) and improves ordering vs MetaAdd.
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_PoolMirror_PositionBatch(array<int> ids, array<vector> positions)
	{
		int n = 0;
		if (ids)
			n = ids.Count();
		HMD_HmdDebug.CliPoolMirrorNetPosRxThrottled(string.Format("SinkRx PositionBatch idCount=%1", n), 500);
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (pool)
			pool.ClientApplyMirrorPositionBatch(ids, positions);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_PoolMirror_ClearAll()
	{
		HMD_HmdDebug.CliPoolMirrorNet("SinkRx ClearAll");
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (pool)
			pool.ClientApplyMirrorClearAll();
	}
}
