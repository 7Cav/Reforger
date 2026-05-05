//------------------------------------------------------------------------------------------------
//! Server-authoritative HMD element registry with reliable metadata + position mirror RPCs (coalesced position batches;
//! full-precision world positions; batched once per simulation step via call queue).
//! **Lifecycle:** `m_pSource` is the entity passed to `ServerRegisterElement`. Pool rows are cleared via
//! `StaticServerRemoveAllPoolRowsForSource` from marker/designation `OnDelete`, and
//! `StaticServerRemoveAllPoolRowsForSourcesInSubtree` from `HMD_LaserMarkingCoreComponent.OnDelete` (plus world designation despawn).
//!
//! **Mirror replication:** clients that need the HUD pool mirror register tokens on the server (`HMD_PoolMirrorSubscription`);
//! updates are sent only to those players via `HMD_PoolMirrorClientSinkComponent` (owner RPC), not broadcast.
class HMD_PoolMirrorRow : Managed
{
	int m_iId;
	EHmdElementKind m_eKind;
	int m_iClassType;
	string m_sText;
	int m_iCode;
	RplId m_Parent0;
	RplId m_Parent1;
	vector m_vPositionWorld;
	//! When true, HUD may draw this row while `HMD_ElementVisualizationInput.ShowTargetDesignations()` is false (see bridge + vehicle marking gate).
	bool m_bShowEvenWhenDesignationHudOff;
	//! Pool HUD dot + label tint from `Color.PackToInt()`; default opaque white.
	int m_iHudColorArgb;
}

class HMD_PoolServerRow : Managed
{
	int m_iId;
	IEntity m_pSource;
	EHmdElementKind m_eKind;
	int m_iClassType;
	string m_sText;
	int m_iCode;
	RplId m_Parent0;
	RplId m_Parent1;
	vector m_vLastSentWorld;
	bool m_bShowEvenWhenDesignationHudOff;
	int m_iHudColorArgb;
}

class HMD_GlobalHmdElementPoolComponentClass : ScriptComponentClass
{
}

class HMD_GlobalHmdElementPoolComponent : ScriptComponent
{
	protected static const int POSITION_BATCH_CHUNK = 16;

	protected static int s_dbgCliPosBatchSample;

	protected int m_iNextElementId = 1;
	protected ref map<int, ref HMD_PoolServerRow> m_mServerRows = new map<int, ref HMD_PoolServerRow>();
	protected ref array<int> m_aServerActiveIds = {};

	protected ref map<int, ref HMD_PoolMirrorRow> m_mMirrorRows = new map<int, ref HMD_PoolMirrorRow>();
	protected ref array<int> m_aMirrorIdOrder = {};

	//! Client only: unreliable position batches may arrive before reliable MetaAdd; stash until the row exists.
	protected ref map<int, vector> m_mCliPendingMirrorWorldPos;

	protected ref map<int, vector> m_mPendingPositions = new map<int, vector>();
	protected ref array<int> m_aPendingPositionIds = {};
	protected bool m_bPositionFlushQueued;

	//! Server: `playerId` -> token -> present (only `true` entries are stored; absent token means off).
	protected ref map<int, ref map<string, bool>> m_mSrvMirrorTokensByPlayer = new map<int, ref map<string, bool>>();

	//! Server: cached list of `playerId` with at least one active mirror token (avoids map key enumeration APIs).
	protected ref array<int> m_aSrvMirrorSubscriberIds = {};

	protected int m_iSrvMirrorReplayRetryPid = -1;

	protected bool m_bSrvMirrorReplayRetryQueued;

	//------------------------------------------------------------------------------------------------
	//! Depth-first search under the game mode entity (pool may live on the mode or a spawned child).
	static HMD_GlobalHmdElementPoolComponent FindPoolInSubtreeRecursive(IEntity root, int depthRemaining)
	{
		if (!root || depthRemaining < 0)
			return null;
		HMD_GlobalHmdElementPoolComponent onSelf = HMD_GlobalHmdElementPoolComponent.Cast(root.FindComponent(HMD_GlobalHmdElementPoolComponent));
		if (onSelf)
			return onSelf;
		IEntity child = root.GetChildren();
		while (child)
		{
			HMD_GlobalHmdElementPoolComponent sub = FindPoolInSubtreeRecursive(child, depthRemaining - 1);
			if (sub)
				return sub;
			child = child.GetSibling();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	static HMD_GlobalHmdElementPoolComponent FindPool()
	{
		BaseGameMode bgm = GetGame().GetGameMode();
		if (!bgm)
			return null;
		IEntity root = bgm;
		if (!root)
			return null;
		return FindPoolInSubtreeRecursive(root, 48);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: remove every pool row whose `m_pSource == source` (any `EHmdElementKind`).
	static void StaticServerRemoveAllPoolRowsForSource(IEntity source)
	{
		if (!Replication.IsServer() || !source)
			return;
		HMD_GlobalHmdElementPoolComponent pool = FindPool();
		if (pool)
			pool.ServerRemoveAllElementsForSource(source);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: remove rows whose `m_pSource` is `subtreeRoot` or any entity under it (vehicle / turret teardown).
	static void StaticServerRemoveAllPoolRowsForSourcesInSubtree(IEntity subtreeRoot)
	{
		if (!Replication.IsServer() || !subtreeRoot)
			return;
		HMD_GlobalHmdElementPoolComponent pool = FindPool();
		if (pool)
			pool.ServerRemoveAllElementsForSourcesUnderEntitySubtree(subtreeRoot);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected bool ServerPlayerHasMirrorTokens(int playerId)
	{
		if (!m_mSrvMirrorTokensByPlayer || !m_mSrvMirrorTokensByPlayer.Contains(playerId))
			return false;
		ref map<string, bool> inner = m_mSrvMirrorTokensByPlayer.Get(playerId);
		return inner && inner.Count() > 0;
	}

	//------------------------------------------------------------------------------------------------
	protected int ServerMirrorTokenCountForPlayer(int playerId)
	{
		if (!m_mSrvMirrorTokensByPlayer || !m_mSrvMirrorTokensByPlayer.Contains(playerId))
			return 0;
		ref map<string, bool> inner = m_mSrvMirrorTokensByPlayer.Get(playerId);
		if (!inner)
			return 0;
		return inner.Count();
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerAddMirrorSubscriberId(int playerId)
	{
		if (playerId < 0)
			return;
		if (m_aSrvMirrorSubscriberIds.Find(playerId) >= 0)
			return;
		m_aSrvMirrorSubscriberIds.Insert(playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerRemoveMirrorSubscriberId(int playerId)
	{
		int idx = m_aSrvMirrorSubscriberIds.Find(playerId);
		if (idx >= 0)
			m_aSrvMirrorSubscriberIds.RemoveOrdered(idx);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: invoked from `HMD_PoolMirrorClientSinkComponent` when a client changes a subscription token.
	void ServerSetClientMirrorToken(int playerId, string token, bool active)
	{
		if (!Replication.IsServer() || playerId < 0 || !token || token.IsEmpty())
			return;

		bool wasSubscribed = ServerPlayerHasMirrorTokens(playerId);

		if (active)
		{
			if (!m_mSrvMirrorTokensByPlayer.Contains(playerId))
				m_mSrvMirrorTokensByPlayer.Set(playerId, new map<string, bool>());
			ref map<string, bool> inner = m_mSrvMirrorTokensByPlayer.Get(playerId);
			inner.Set(token, true);
		}
		else
		{
			if (m_mSrvMirrorTokensByPlayer.Contains(playerId))
			{
				ref map<string, bool> inner = m_mSrvMirrorTokensByPlayer.Get(playerId);
				inner.Remove(token);
				if (inner.Count() == 0)
					m_mSrvMirrorTokensByPlayer.Remove(playerId);
			}
		}

		bool nowSubscribed = ServerPlayerHasMirrorTokens(playerId);

		if (!wasSubscribed && nowSubscribed)
		{
			ServerAddMirrorSubscriberId(playerId);
			ServerReplayFullMirrorToPlayer(playerId);
		}
		else if (wasSubscribed && !nowSubscribed)
		{
			ServerSendMirrorClearToPlayer(playerId);
			ServerRemoveMirrorSubscriberId(playerId);
		}

		HMD_HmdDebug.SrvPoolMirrorNet(string.Format("SubToken playerId=%1 token=\"%2\" active=%3 wasSubscribed=%4 nowSubscribed=%5 tokenCount=%6 subscriberSlots=%7", playerId, token, active, wasSubscribed, nowSubscribed, ServerMirrorTokenCountForPlayer(playerId), m_aSrvMirrorSubscriberIds.Count()));
	}

	//------------------------------------------------------------------------------------------------
	//! Server: drop all subscription tokens for a disconnecting player.
	void ServerClearMirrorSubscriptionForPlayer(int playerId)
	{
		if (!Replication.IsServer() || playerId < 0)
			return;
		HMD_HmdDebug.SrvPoolMirrorNet(string.Format("SubDisconnectClear playerId=%1 hadTokenMap=%2 wasInFanoutList=%3", playerId, m_mSrvMirrorTokensByPlayer && m_mSrvMirrorTokensByPlayer.Contains(playerId), m_aSrvMirrorSubscriberIds.Find(playerId) >= 0));
		if (m_mSrvMirrorTokensByPlayer && m_mSrvMirrorTokensByPlayer.Contains(playerId))
			m_mSrvMirrorTokensByPlayer.Remove(playerId);
		ServerRemoveMirrorSubscriberId(playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerSendMirrorClearToPlayer(int playerId)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		HMD_PoolMirrorClientSinkComponent sink = HMD_PoolMirrorClientSinkComponent.FindSinkForPlayerId(pm, playerId);
		HMD_HmdDebug.SrvPoolMirrorNet(string.Format("SubClearMirror playerId=%1 sinkFound=%2", playerId, sink != null));
		if (sink)
			sink.Authority_PoolMirror_ClearAll();
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerQueueMirrorReplayRetry(int playerId)
	{
		if (!GetGame())
			return;
		m_iSrvMirrorReplayRetryPid = playerId;
		if (m_bSrvMirrorReplayRetryQueued)
			return;
		m_bSrvMirrorReplayRetryQueued = true;
		HMD_HmdDebug.SrvPoolMirrorNet(string.Format("ReplayRetrySchedule playerId=%1 delayMs=200", playerId));
		GetGame().GetCallqueue().CallLater(ServerRunMirrorReplayRetry, 200, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerRunMirrorReplayRetry()
	{
		m_bSrvMirrorReplayRetryQueued = false;
		int pid = m_iSrvMirrorReplayRetryPid;
		m_iSrvMirrorReplayRetryPid = -1;
		if (!Replication.IsServer() || pid < 0)
			return;
		HMD_HmdDebug.SrvPoolMirrorNet(string.Format("ReplayRetryRun playerId=%1", pid));
		ServerReplayFullMirrorToPlayer(pid);
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerReplayFullMirrorToPlayer(int playerId)
	{
		if (!Replication.IsServer())
			return;
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		HMD_PoolMirrorClientSinkComponent sink = HMD_PoolMirrorClientSinkComponent.FindSinkForPlayerId(pm, playerId);
		if (!sink)
		{
			HMD_HmdDebug.SrvPoolMirrorNet(string.Format("ReplayNoSink queueRetry playerId=%1", playerId));
			ServerQueueMirrorReplayRetry(playerId);
			return;
		}

		HMD_HmdDebug.SrvStorage(string.Format("ReplayFullMirror playerId=%1 serverActiveRows=%2", playerId, m_aServerActiveIds.Count()));
		HMD_HmdDebug.SrvPoolMirrorNet(string.Format("ReplayFullMirrorBegin playerId=%1 rows=%2 fanoutSubscribers=%3", playerId, m_aServerActiveIds.Count(), m_aSrvMirrorSubscriberIds.Count()));

		array<int> replayIds = new array<int>();
		array<vector> replayQs = new array<vector>();
		foreach (int id : m_aServerActiveIds)
		{
			HMD_PoolServerRow row = m_mServerRows.Get(id);
			if (!row)
				continue;
			sink.Authority_PoolMirror_MetaAdd(id, row.m_eKind, row.m_iClassType, row.m_sText, row.m_iCode, row.m_Parent0, row.m_Parent1, row.m_bShowEvenWhenDesignationHudOff);
			sink.Authority_PoolMirror_MetaHudColor(id, row.m_iHudColorArgb);
			replayIds.Insert(id);
			replayQs.Insert(row.m_vLastSentWorld);
		}
		ServerSendPositionChunksToSink(sink, replayIds, replayQs);
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerSendPositionChunksToSink(HMD_PoolMirrorClientSinkComponent sink, notnull array<int> ids, notnull array<vector> qs)
	{
		if (!sink)
			return;
		int at = 0;
		while (at < ids.Count())
		{
			array<int> chunkIds = new array<int>();
			array<vector> chunkQs = new array<vector>();
			for (int j = 0; j < POSITION_BATCH_CHUNK && at < ids.Count(); j++)
			{
				chunkIds.Insert(ids[at]);
				chunkQs.Insert(qs[at]);
				at++;
			}
			if (chunkIds.Count() < 1)
				break;
			sink.Authority_PoolMirror_PositionBatch(chunkIds, chunkQs);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerDispatchMetaAddToSubscribers(int id, EHmdElementKind kind, int classType, string text, int code, RplId parent0, RplId parent1, bool showEvenWhenDesignationHudOff, int hudColorArgb)
	{
		if (!Replication.IsServer())
			return;
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		int subs = m_aSrvMirrorSubscriberIds.Count();
		foreach (int pid : m_aSrvMirrorSubscriberIds)
		{
			HMD_PoolMirrorClientSinkComponent sink = HMD_PoolMirrorClientSinkComponent.FindSinkForPlayerId(pm, pid);
			if (!sink)
				continue;
			sink.Authority_PoolMirror_MetaAdd(id, kind, classType, text, code, parent0, parent1, showEvenWhenDesignationHudOff);
			sink.Authority_PoolMirror_MetaHudColor(id, hudColorArgb);
		}
		HMD_HmdDebug.SrvPoolMirrorNet(string.Format("FanOut MetaAdd id=%1 kind=%2 subs=%3 text=\"%4\"", id, typename.EnumToString(EHmdElementKind, kind), subs, text));
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerDispatchMetaRemoveToSubscribers(int id)
	{
		if (!Replication.IsServer())
			return;
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		int subs = m_aSrvMirrorSubscriberIds.Count();
		foreach (int pid : m_aSrvMirrorSubscriberIds)
		{
			HMD_PoolMirrorClientSinkComponent sink = HMD_PoolMirrorClientSinkComponent.FindSinkForPlayerId(pm, pid);
			if (sink)
				sink.Authority_PoolMirror_MetaRemove(id);
		}
		HMD_HmdDebug.SrvPoolMirrorNet(string.Format("FanOut MetaRemove id=%1 subs=%2", id, subs));
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerDispatchMetaCodeToSubscribers(int id, int newCode)
	{
		if (!Replication.IsServer())
			return;
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		int subs = m_aSrvMirrorSubscriberIds.Count();
		foreach (int pid : m_aSrvMirrorSubscriberIds)
		{
			HMD_PoolMirrorClientSinkComponent sink = HMD_PoolMirrorClientSinkComponent.FindSinkForPlayerId(pm, pid);
			if (sink)
				sink.Authority_PoolMirror_MetaCode(id, newCode);
		}
		HMD_HmdDebug.SrvPoolMirrorNet(string.Format("FanOut MetaCode id=%1 code=%2 subs=%3", id, newCode, subs));
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerDispatchMetaTextToSubscribers(int id, string text)
	{
		if (!Replication.IsServer())
			return;
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		int subs = m_aSrvMirrorSubscriberIds.Count();
		foreach (int pid : m_aSrvMirrorSubscriberIds)
		{
			HMD_PoolMirrorClientSinkComponent sink = HMD_PoolMirrorClientSinkComponent.FindSinkForPlayerId(pm, pid);
			if (sink)
				sink.Authority_PoolMirror_MetaText(id, text);
		}
		HMD_HmdDebug.SrvPoolMirrorNet(string.Format("FanOut MetaText id=%1 subs=%2 text=\"%3\"", id, subs, text));
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerDispatchPositionBatchToSubscribers(notnull array<int> ids, notnull array<vector> qs)
	{
		if (!Replication.IsServer())
			return;
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		int subs = m_aSrvMirrorSubscriberIds.Count();
		foreach (int pid : m_aSrvMirrorSubscriberIds)
		{
			HMD_PoolMirrorClientSinkComponent sink = HMD_PoolMirrorClientSinkComponent.FindSinkForPlayerId(pm, pid);
			if (!sink)
				continue;
			ServerSendPositionChunksToSink(sink, ids, qs);
		}
		HMD_HmdDebug.SrvPoolMirrorNetPosFanoutThrottled(string.Format("FanOut PositionBatch idCount=%1 subs=%2", ids.Count(), subs), 500);
	}

	//------------------------------------------------------------------------------------------------
	protected void EnqueueServerPositionUpdate(int id, vector worldPos)
	{
		if (!Replication.IsServer())
			return;
		m_mPendingPositions.Set(id, worldPos);
		if (m_aPendingPositionIds.Find(id) < 0)
			m_aPendingPositionIds.Insert(id);
		SchedulePositionFlush();
	}

	//------------------------------------------------------------------------------------------------
	protected void SchedulePositionFlush()
	{
		if (!Replication.IsServer() || m_bPositionFlushQueued)
			return;
		m_bPositionFlushQueued = true;
		GetGame().GetCallqueue().CallLater(FlushPendingPositions, 0, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void FlushPendingPositions()
	{
		m_bPositionFlushQueued = false;
		if (!Replication.IsServer())
			return;
		if (!m_aPendingPositionIds || m_aPendingPositionIds.Count() == 0)
			return;

		array<int> batchIds = new array<int>();
		array<vector> batchQs = new array<vector>();
		for (int i = 0; i < m_aPendingPositionIds.Count(); i++)
		{
			int id = m_aPendingPositionIds[i];
			if (!m_mPendingPositions.Contains(id))
				continue;
			batchIds.Insert(id);
			batchQs.Insert(m_mPendingPositions.Get(id));
		}
		m_mPendingPositions.Clear();
		m_aPendingPositionIds.Clear();

		SendPositionBatchChunks(batchIds, batchQs);
	}

	//------------------------------------------------------------------------------------------------
	protected void SendPositionBatchChunks(notnull array<int> ids, notnull array<vector> qs)
	{
		int at = 0;
		while (at < ids.Count())
		{
			array<int> chunkIds = new array<int>();
			array<vector> chunkQs = new array<vector>();
			for (int j = 0; j < POSITION_BATCH_CHUNK && at < ids.Count(); j++)
			{
				chunkIds.Insert(ids[at]);
				chunkQs.Insert(qs[at]);
				at++;
			}
			if (chunkIds.Count() < 1)
				break;
			if (Replication.IsServer())
				ServerDispatchPositionBatchToSubscribers(chunkIds, chunkQs);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearPendingPositionForId(int id)
	{
		if (m_mPendingPositions && m_mPendingPositions.Contains(id))
			m_mPendingPositions.Remove(id);
		int fi = m_aPendingPositionIds.Find(id);
		if (fi >= 0)
			m_aPendingPositionIds.RemoveOrdered(fi);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: allocate a new pool row and broadcast metadata to clients.
	int ServerRegisterElement(IEntity source, EHmdElementKind kind, int classType, string text, int code, RplId parent0, RplId parent1, bool showEvenWhenDesignationHudOff, int hudColorArgb)
	{
		if (!Replication.IsServer())
			return -1;

		int id = m_iNextElementId;
		m_iNextElementId++;

		HMD_PoolServerRow row = new HMD_PoolServerRow();
		row.m_iId = id;
		row.m_pSource = source;
		row.m_eKind = kind;
		row.m_iClassType = classType;
		row.m_sText = text;
		row.m_iCode = code;
		row.m_Parent0 = parent0;
		row.m_Parent1 = parent1;
		row.m_vLastSentWorld = vector.Zero;
		row.m_bShowEvenWhenDesignationHudOff = showEvenWhenDesignationHudOff;
		row.m_iHudColorArgb = hudColorArgb;

		m_mServerRows.Set(id, row);
		m_aServerActiveIds.Insert(id);

		HMD_HmdDebug.SrvStorage(string.Format("Register id=%1 kind=%2 class=%3 text=\"%4\" code=%5 activeServerRows=%6", id, typename.EnumToString(EHmdElementKind, kind), classType, text, code, m_aServerActiveIds.Count()));

		ServerDispatchMetaAddToSubscribers(id, kind, classType, text, code, parent0, parent1, showEvenWhenDesignationHudOff, hudColorArgb);

		vector q = vector.Zero;
		if (source)
			q = source.GetOrigin();
		row.m_vLastSentWorld = q;
		EnqueueServerPositionUpdate(id, q);

		return id;
	}

	//------------------------------------------------------------------------------------------------
	void ServerRemoveElement(int id)
	{
		if (!Replication.IsServer())
			return;
		if (!m_mServerRows.Contains(id))
			return;

		m_mServerRows.Remove(id);
		int idx = m_aServerActiveIds.Find(id);
		if (idx >= 0)
			m_aServerActiveIds.RemoveOrdered(idx);

		ClearPendingPositionForId(id);

		HMD_HmdDebug.SrvStorage(string.Format("Remove id=%1 activeServerRows=%2", id, m_aServerActiveIds.Count()));

		ServerDispatchMetaRemoveToSubscribers(id);
	}

	//------------------------------------------------------------------------------------------------
	//! Removes every server row registered with `source` and `kind` (prefab destroyed / component lost its pool id).
	void ServerRemoveElementsForSourceAndKind(IEntity source, EHmdElementKind kind)
	{
		if (!Replication.IsServer() || !source)
			return;
		array<int> hitIds = {};
		int n = m_aServerActiveIds.Count();
		for (int i = 0; i < n; i++)
		{
			int id = m_aServerActiveIds[i];
			HMD_PoolServerRow row = m_mServerRows.Get(id);
			if (!row)
				continue;
			if (row.m_pSource != source)
				continue;
			if (row.m_eKind != kind)
				continue;
			hitIds.Insert(id);
		}
		for (int j = 0; j < hitIds.Count(); j++)
			ServerRemoveElement(hitIds[j]);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: remove every pool row with `m_pSource == source` regardless of kind (marker + designation on same entity).
	void ServerRemoveAllElementsForSource(IEntity source)
	{
		if (!Replication.IsServer() || !source)
			return;
		array<int> hitIds = {};
		int n = m_aServerActiveIds.Count();
		for (int i = 0; i < n; i++)
		{
			int id = m_aServerActiveIds[i];
			HMD_PoolServerRow row = m_mServerRows.Get(id);
			if (!row)
				continue;
			if (row.m_pSource != source)
				continue;
			hitIds.Insert(id);
		}
		for (int j = 0; j < hitIds.Count(); j++)
			ServerRemoveElement(hitIds[j]);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: remove rows whose `m_pSource` lies on or under `subtreeRoot` in the entity hierarchy.
	void ServerRemoveAllElementsForSourcesUnderEntitySubtree(IEntity subtreeRoot)
	{
		if (!Replication.IsServer() || !subtreeRoot)
			return;
		array<int> hitIds = {};
		int n = m_aServerActiveIds.Count();
		for (int i = 0; i < n; i++)
		{
			int id = m_aServerActiveIds[i];
			HMD_PoolServerRow row = m_mServerRows.Get(id);
			if (!row || !row.m_pSource)
				continue;
			if (!HMD_EntityHmdHelpers.EntityIsOrUnderRoot(row.m_pSource, subtreeRoot))
				continue;
			hitIds.Insert(id);
		}
		for (int j = 0; j < hitIds.Count(); j++)
			ServerRemoveElement(hitIds[j]);
	}

	//------------------------------------------------------------------------------------------------
	void ServerUpdateElementCode(int id, int code)
	{
		if (!Replication.IsServer())
			return;
		HMD_PoolServerRow row = m_mServerRows.Get(id);
		if (!row)
			return;
		if (row.m_iCode == code)
			return;
		row.m_iCode = code;

		HMD_HmdDebug.SrvStorage(string.Format("UpdateCode id=%1 code=%2", id, code));

		ServerDispatchMetaCodeToSubscribers(id, code);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: update pool row HUD text (`m_sText`) for informational / designation labels.
	void ServerUpdateElementText(int id, string text)
	{
		if (!Replication.IsServer())
			return;
		HMD_PoolServerRow row = m_mServerRows.Get(id);
		if (!row)
			return;
		if (row.m_sText == text)
			return;
		row.m_sText = text;

		HMD_HmdDebug.SrvStorage(string.Format("UpdateText id=%1 text=\"%2\"", id, text));

		ServerDispatchMetaTextToSubscribers(id, text);
	}

	//------------------------------------------------------------------------------------------------
	void ServerSetElementWorldPosition(int id, vector worldPos)
	{
		if (!Replication.IsServer())
			return;
		HMD_PoolServerRow row = m_mServerRows.Get(id);
		if (!row)
			return;

		vector q = worldPos;
		vector delta = q - row.m_vLastSentWorld;
		//! Suppress duplicate sends when origin is unchanged (sub-mm jitter still syncs).
		if (delta.LengthSq() < 1e-12)
			return;
		row.m_vLastSentWorld = q;

		EnqueueServerPositionUpdate(id, q);
	}

	//------------------------------------------------------------------------------------------------
	void ClientApplyMirrorMetaAdd(int id, EHmdElementKind kind, int classType, string text, int code, RplId parent0, RplId parent1, bool showEvenWhenDesignationHudOff)
	{
		if (m_mMirrorRows.Contains(id))
		{
			HMD_PoolMirrorRow mirror = m_mMirrorRows.Get(id);
			if (mirror)
			{
				mirror.m_eKind = kind;
				mirror.m_iClassType = classType;
				mirror.m_sText = text;
				mirror.m_iCode = code;
				mirror.m_Parent0 = parent0;
				mirror.m_Parent1 = parent1;
				mirror.m_bShowEvenWhenDesignationHudOff = showEvenWhenDesignationHudOff;
			}
			HMD_HmdDebug.CliPoolMirror(string.Format("MetaAdd upsert id=%1 kind=%2 code=%3 text=\"%4\"", id, typename.EnumToString(EHmdElementKind, kind), code, text));
			CliApplyPendingMirrorWorldPosIfAny(id);
			return;
		}
		HMD_PoolMirrorRow newRow = new HMD_PoolMirrorRow();
		newRow.m_iId = id;
		newRow.m_eKind = kind;
		newRow.m_iClassType = classType;
		newRow.m_sText = text;
		newRow.m_iCode = code;
		newRow.m_Parent0 = parent0;
		newRow.m_Parent1 = parent1;
		newRow.m_vPositionWorld = vector.Zero;
		newRow.m_bShowEvenWhenDesignationHudOff = showEvenWhenDesignationHudOff;
		newRow.m_iHudColorArgb = 0xFFFFFFFF;
		m_mMirrorRows.Set(id, newRow);
		m_aMirrorIdOrder.Insert(id);
		HMD_HmdDebug.CliPoolMirror(string.Format("MetaAdd new id=%1 kind=%2 class=%3 code=%4 text=\"%5\" mirrorOrder=%6", id, typename.EnumToString(EHmdElementKind, kind), classType, code, text, m_aMirrorIdOrder.Count()));
		CliApplyPendingMirrorWorldPosIfAny(id);
	}

	//------------------------------------------------------------------------------------------------
	void ClientApplyMirrorMetaHudColor(int id, int hudColorArgb)
	{
		HMD_PoolMirrorRow mirror = m_mMirrorRows.Get(id);
		if (mirror)
			mirror.m_iHudColorArgb = hudColorArgb;
	}

	//------------------------------------------------------------------------------------------------
	void ClientApplyMirrorMetaRemove(int id)
	{
		HMD_HmdDebug.CliPoolMirror(string.Format("MetaRemove id=%1", id));
		if (m_mCliPendingMirrorWorldPos && m_mCliPendingMirrorWorldPos.Contains(id))
			m_mCliPendingMirrorWorldPos.Remove(id);
		if (m_mMirrorRows.Contains(id))
			m_mMirrorRows.Remove(id);
		int oi = m_aMirrorIdOrder.Find(id);
		if (oi >= 0)
			m_aMirrorIdOrder.RemoveOrdered(oi);
	}

	//------------------------------------------------------------------------------------------------
	void ClientApplyMirrorMetaCode(int id, int newCode)
	{
		HMD_HmdDebug.CliPoolMirror(string.Format("MetaCode id=%1 code=%2", id, newCode));
		HMD_PoolMirrorRow mirror = m_mMirrorRows.Get(id);
		if (mirror)
			mirror.m_iCode = newCode;
	}

	//------------------------------------------------------------------------------------------------
	void ClientApplyMirrorMetaText(int id, string text)
	{
		HMD_HmdDebug.CliPoolMirror(string.Format("MetaText id=%1 text=\"%2\"", id, text));
		HMD_PoolMirrorRow mirror = m_mMirrorRows.Get(id);
		if (mirror)
			mirror.m_sText = text;
	}

	//------------------------------------------------------------------------------------------------
	protected void CliApplyPendingMirrorWorldPosIfAny(int id)
	{
		if (!m_mCliPendingMirrorWorldPos || !m_mCliPendingMirrorWorldPos.Contains(id))
			return;
		HMD_PoolMirrorRow mirror = m_mMirrorRows.Get(id);
		if (!mirror)
			return;
		mirror.m_vPositionWorld = m_mCliPendingMirrorWorldPos.Get(id);
		m_mCliPendingMirrorWorldPos.Remove(id);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClientApplyMirrorPositionSingle(int id, vector worldPos)
	{
		HMD_PoolMirrorRow mirror = m_mMirrorRows.Get(id);
		if (mirror)
		{
			mirror.m_vPositionWorld = worldPos;
			return;
		}
		if (!m_mCliPendingMirrorWorldPos)
			m_mCliPendingMirrorWorldPos = new map<int, vector>();
		m_mCliPendingMirrorWorldPos.Set(id, worldPos);
	}

	//------------------------------------------------------------------------------------------------
	void ClientApplyMirrorPositionBatch(array<int> ids, array<vector> positions)
	{
		if (!ids || !positions)
			return;
		int n = ids.Count();
		if (positions.Count() < n)
			n = positions.Count();
		s_dbgCliPosBatchSample++;
		if (n > 0 && s_dbgCliPosBatchSample % 45 == 1)
			HMD_HmdDebug.CliPoolMirror(string.Format("PositionBatch n=%1 (sampled)", n));
		for (int i = 0; i < n; i++)
			ClientApplyMirrorPositionSingle(ids[i], positions[i]);
	}

	//------------------------------------------------------------------------------------------------
	void ClientApplyMirrorClearAll()
	{
		HMD_HmdDebug.CliPoolMirror("MirrorClearAll");
		m_mMirrorRows.Clear();
		m_aMirrorIdOrder.Clear();
		if (m_mCliPendingMirrorWorldPos)
			m_mCliPendingMirrorWorldPos.Clear();
	}

	//------------------------------------------------------------------------------------------------
	void GetMirrorRows(array<ref HMD_PoolMirrorRow> outRows)
	{
		outRows.Clear();
		foreach (int id : m_aMirrorIdOrder)
		{
			HMD_PoolMirrorRow row = m_mMirrorRows.Get(id);
			if (row)
				outRows.Insert(row);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Legacy no-op on clients; pool mirror is subscription-driven. Listen-host server still has no mirror traffic here.
	void AskRequestFullMirrorFromClient()
	{
	}
}
