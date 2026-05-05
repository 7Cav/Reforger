//------------------------------------------------------------------------------------------------
//! Placeable IFF beacon: replicated text index + number (scroll while OFF), ON/OFF, battery while transmitting.
//! When **placed** and **ON** with battery, registers **informational class 0** (IFF) in `HMD_GlobalHmdElementPoolComponent`.
[ComponentEditorProps(category: "HMD", description: "Attachable IFF beacon; global HMD pool class 0")]
class HMD_IffAttachableBeaconComponentClass : ScriptComponentClass
{
}

class HMD_IffAttachableBeaconComponent : ScriptComponent
{
	protected static const float BEACON_TOTAL_SECONDS = 1800.0;
	protected static const int TEXT_COUNT = 5;
	protected static const float BATTERY_DRAIN_DT_CAP = 5.0;

	[Attribute("1800", UIWidgets.Auto, "Battery life in seconds while ON after placement.", category: "HMD")]
	protected float m_fBatteryLifeSeconds;

	[Attribute("75", UIWidgets.Auto, "Server-only: after placement notify, wait this many ms then refresh pool from authority GetOrigin (physics/replication settle). 0 = skip delayed resample.", category: "HMD Placement")]
	protected float m_fPostPlacementSampleDelayMs;

	[RplProp(onRplName: "OnBeaconStateReplicated")]
	protected bool m_bBeaconActive;

	[RplProp(onRplName: "OnBeaconStateReplicated")]
	protected int m_iTextIndex;

	[RplProp(onRplName: "OnBeaconStateReplicated")]
	protected int m_iNumber;

	[RplProp(onRplName: "OnBeaconStateReplicated")]
	protected float m_fBatterySecondsRemaining;

	[RplProp(onRplName: "OnPlacedStateReplicated")]
	protected bool m_bPlacedInWorld;

	protected int m_iPoolElementId = -1;
	protected string m_sLastPoolText = string.Empty;

	protected float m_fBatteryBumpAccum;
	protected int m_iDbgLastIffPoolPosLogMs;

	//! Client: `SCR_PlaceableInventoryItemComponent` can fire multiple hooks before physics settles; coalesce to one notify.
	protected static const int NOTIFY_PLACED_DEBOUNCE_MS = 100;

	//------------------------------------------------------------------------------------------------
	static HMD_IffAttachableBeaconComponent FindOnEntity(IEntity owner)
	{
		if (!owner)
			return null;
		return HMD_IffAttachableBeaconComponent.Cast(owner.FindComponent(HMD_IffAttachableBeaconComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected string BuildMarkerLabel()
	{
		string t = "HLS";
		switch (m_iTextIndex)
		{
			case 0: { t = "HLS"; break; }
			case 1: { t = "VKY"; break; }
			case 2: { t = "BSH"; break; }
			case 3: { t = "BND"; break; }
			case 4: { t = "MSF"; break; }
		}
		int n = m_iNumber;
		if (n < 1)
			n = 1;
		if (n > 7)
			n = 7;
		return string.Format("%1-%2", t, n);
	}

	//------------------------------------------------------------------------------------------------
	protected void AuthorityTickDeploymentGates(IEntity owner)
	{
		if (!owner)
			return;
		InventoryItemComponent inv = InventoryItemComponent.Cast(owner.FindComponent(InventoryItemComponent));
		if (inv && inv.GetParentSlot() && m_bPlacedInWorld)
		{
			m_bPlacedInWorld = false;
			if (Replication.IsRunning())
				Replication.BumpMe();
			HmdServerRefreshPool();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Authority-only: one extra `HmdServerRefreshPool` after placeable/physics settle.
	protected void HmdScheduleServerPostPlacementResample()
	{
		if (!Replication.IsServer() || !GetGame())
			return;
		float d = m_fPostPlacementSampleDelayMs;
		if (d <= 0)
			return;
		int delayMs = (int)d;
		if (delayMs < 1)
			delayMs = 1;
		GetGame().GetCallqueue().Remove(HmdServerPostPlacementResample);
		GetGame().GetCallqueue().CallLater(HmdServerPostPlacementResample, delayMs, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdServerPostPlacementResample()
	{
		if (!Replication.IsServer())
			return;
		IEntity owner = GetOwner();
		if (!owner || !owner.GetWorld())
			return;
		if (!m_bPlacedInWorld)
			return;
		HMD_HmdDebug.SrvIffBeaconPlace(string.Format("PostPlacementResample origin=%1", owner.GetOrigin()));
		HmdServerRefreshPool();
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdServerRemovePoolRow()
	{
		if (!Replication.IsServer() || m_iPoolElementId < 0)
			return;
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (pool)
			pool.ServerRemoveElement(m_iPoolElementId);
		m_iPoolElementId = -1;
		m_sLastPoolText = string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdServerRefreshPool()
	{
		if (!Replication.IsServer())
			return;
		IEntity owner = GetOwner();
		//! Not in world (inventory, pending delete, etc.): always drop the pool row.
		if (!owner || !owner.GetWorld())
		{
			HmdServerRemovePoolRow();
			return;
		}
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (!pool)
			return;

		bool show = m_bPlacedInWorld && m_bBeaconActive && m_fBatterySecondsRemaining > 0.0;
		if (!show)
		{
			HmdServerRemovePoolRow();
			return;
		}

		string label = BuildMarkerLabel();
		RplId p0 = RplId.Invalid();
		RplId p1 = RplId.Invalid();
		IEntity p = owner.GetParent();
		if (p)
		{
			RplComponent r0 = RplComponent.Cast(p.FindComponent(RplComponent));
			if (r0)
				p0 = Replication.FindId(r0);
			IEntity p2 = p.GetParent();
			if (p2)
			{
				RplComponent r1 = RplComponent.Cast(p2.FindComponent(RplComponent));
				if (r1)
					p1 = Replication.FindId(r1);
			}
		}

		if (m_iPoolElementId < 0)
		{
			m_iPoolElementId = pool.ServerRegisterElement(owner, EHmdElementKind.INFORMATIONAL, 0, label, 0, p0, p1, false, 0xFFFFFFFF);
			m_sLastPoolText = label;
			HMD_HmdDebug.SrvIffBeaconPlace(string.Format("pool register id=%1 label=%2 origin=%3 parent0=%4", m_iPoolElementId, label, owner.GetOrigin(), p0));
			return;
		}

		if (label != m_sLastPoolText)
		{
			pool.ServerUpdateElementText(m_iPoolElementId, label);
			m_sLastPoolText = label;
		}
		pool.ServerSetElementWorldPosition(m_iPoolElementId, owner.GetOrigin());
		int nowMs = System.GetTickCount();
		if (m_iDbgLastIffPoolPosLogMs == 0 || nowMs - m_iDbgLastIffPoolPosLogMs >= 500)
		{
			m_iDbgLastIffPoolPosLogMs = nowMs;
			HMD_HmdDebug.SrvIffBeaconPlace(string.Format("pool pos tick id=%1 origin=%2", m_iPoolElementId, owner.GetOrigin()));
		}
	}

	//------------------------------------------------------------------------------------------------
	void OnBeaconStateReplicated()
	{
	}

	//------------------------------------------------------------------------------------------------
	void OnPlacedStateReplicated()
	{
	}

	//------------------------------------------------------------------------------------------------
	//! Call from placeable hooks (`PlacementDone` / `OnPlacedOnGround` may fire more than once). Debounces so we register placed state once after vanilla placement settles.
	void ScheduleNotifyPlacedInWorld()
	{
		if (!GetGame())
			return;
		GetGame().GetCallqueue().Remove(HmdDeferredNotifyPlacedInWorld);
		GetGame().GetCallqueue().CallLater(HmdDeferredNotifyPlacedInWorld, NOTIFY_PLACED_DEBOUNCE_MS, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdDeferredNotifyPlacedInWorld()
	{
		IEntity ow = GetOwner();
		if (!ow || !ow.GetWorld())
			return;
		NotifyPlacedInWorld();
	}

	//------------------------------------------------------------------------------------------------
	//! Pool / placed flags only — entity pose is owned by vanilla placement + replication; server uses authority `GetOrigin()` when refreshing the pool.
	void NotifyPlacedInWorld()
	{
		if (!Replication.IsRunning() || Replication.IsServer())
		{
			ApplyPlacedInWorldOnAuthority();
		}
		else
		{
			HMD_HmdDebug.CliIffBeaconPlace("NotifyPlacedInWorld client->Rpc (no prefab snap)");
			Rpc(RpcAsk_NotifyPlacedInWorld);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_NotifyPlacedInWorld()
	{
		IEntity ow = GetOwner();
		vector o = vector.Zero;
		if (ow)
			o = ow.GetOrigin();
		HMD_HmdDebug.SrvIffBeaconPlace(string.Format("RpcAsk_NotifyPlacedInWorld server origin=%1", o));
		ApplyPlacedInWorldOnAuthority();
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyPlacedInWorldOnAuthority()
	{
		m_bPlacedInWorld = true;
		if (Replication.IsRunning())
			Replication.BumpMe();
		IEntity ow = GetOwner();
		vector o = vector.Zero;
		if (ow)
			o = ow.GetOrigin();
		HMD_HmdDebug.SrvIffBeaconPlace(string.Format("ApplyPlacedInWorldOnAuthority placed=1 origin=%1 inWorld=%2", o, ow && ow.GetWorld()));
		HmdServerRefreshPool();
		HmdScheduleServerPostPlacementResample();
	}

	//------------------------------------------------------------------------------------------------
	bool IsBeaconActive()
	{
		return m_bBeaconActive;
	}

	//------------------------------------------------------------------------------------------------
	bool IsBeaconPlacedInWorld()
	{
		return m_bPlacedInWorld;
	}

	//------------------------------------------------------------------------------------------------
	//! Scroll + turn ON: only after placement, while OFF, with battery.
	bool CanConfigure()
	{
		return m_bPlacedInWorld && !m_bBeaconActive && m_fBatterySecondsRemaining > 0.0;
	}

	//------------------------------------------------------------------------------------------------
	float GetBatteryFraction01()
	{
		float total = m_fBatteryLifeSeconds;
		if (total <= 0.0)
			total = BEACON_TOTAL_SECONDS;
		float f = m_fBatterySecondsRemaining / total;
		if (f < 0.0)
			f = 0.0;
		if (f > 1.0)
			f = 1.0;
		return f;
	}

	//------------------------------------------------------------------------------------------------
	string GetPreviewLabel()
	{
		return BuildMarkerLabel();
	}

	//------------------------------------------------------------------------------------------------
	string GetPreviewTextCode()
	{
		string t = "HLS";
		switch (m_iTextIndex)
		{
			case 0: { t = "HLS"; break; }
			case 1: { t = "VKY"; break; }
			case 2: { t = "BSH"; break; }
			case 3: { t = "BND"; break; }
			case 4: { t = "MSF"; break; }
		}
		return t;
	}

	//------------------------------------------------------------------------------------------------
	string GetPreviewNumberString()
	{
		int n = m_iNumber;
		if (n < 1)
			n = 1;
		if (n > 7)
			n = 7;
		return string.Format("%1", n);
	}

	//------------------------------------------------------------------------------------------------
	float GetTextIndexNormalized01()
	{
		int idx = m_iTextIndex;
		if (idx < 0)
			idx = 0;
		if (idx >= TEXT_COUNT)
			idx = TEXT_COUNT - 1;
		float denom = TEXT_COUNT - 1;
		if (denom <= 0.0)
			return 0.0;
		return idx / denom;
	}

	//------------------------------------------------------------------------------------------------
	float PredictTextIndexNormalized01AfterDir(int dir)
	{
		if (!m_bPlacedInWorld || m_bBeaconActive || m_fBatterySecondsRemaining <= 0.0)
			return GetTextIndexNormalized01();
		int idx = m_iTextIndex;
		if (dir > 0)
		{
			idx++;
			if (idx >= TEXT_COUNT)
				idx = 0;
		}
		else
		{
			idx--;
			if (idx < 0)
				idx = TEXT_COUNT - 1;
		}
		float denom = TEXT_COUNT - 1;
		if (denom <= 0.0)
			return 0.0;
		return idx / denom;
	}

	//------------------------------------------------------------------------------------------------
	float GetNumberNormalized01()
	{
		int n = m_iNumber;
		if (n < 1)
			n = 1;
		if (n > 7)
			n = 7;
		return (n - 1) / 6.0;
	}

	//------------------------------------------------------------------------------------------------
	float PredictNumberNormalized01AfterDir(int dir)
	{
		if (!m_bPlacedInWorld || m_bBeaconActive || m_fBatterySecondsRemaining <= 0.0)
			return GetNumberNormalized01();
		int n = m_iNumber;
		if (dir > 0)
		{
			n++;
			if (n > 7)
				n = 1;
		}
		else
		{
			n--;
			if (n < 1)
				n = 7;
		}
		return (n - 1) / 6.0;
	}

	//------------------------------------------------------------------------------------------------
	void ServerApplyTextIndexFromNormalized01(float t)
	{
		if (Replication.IsRunning() && !Replication.IsServer())
			return;
		if (!m_bPlacedInWorld || m_bBeaconActive || m_fBatterySecondsRemaining <= 0.0)
			return;
		if (t < 0.0)
			t = 0.0;
		if (t > 1.0)
			t = 1.0;
		float denom = TEXT_COUNT - 1;
		int idx = 0;
		if (denom > 0.0)
			idx = Math.Round(t * denom);
		if (idx < 0)
			idx = 0;
		if (idx >= TEXT_COUNT)
			idx = TEXT_COUNT - 1;
		if (m_iTextIndex == idx)
			return;
		m_iTextIndex = idx;
		if (Replication.IsRunning())
			Replication.BumpMe();
		HmdServerRefreshPool();
	}

	//------------------------------------------------------------------------------------------------
	void ServerApplyNumberFromNormalized01(float t)
	{
		if (Replication.IsRunning() && !Replication.IsServer())
			return;
		if (!m_bPlacedInWorld || m_bBeaconActive || m_fBatterySecondsRemaining <= 0.0)
			return;
		if (t < 0.0)
			t = 0.0;
		if (t > 1.0)
			t = 1.0;
		int n = Math.Round(t * 6.0) + 1;
		if (n < 1)
			n = 1;
		if (n > 7)
			n = 7;
		if (m_iNumber == n)
			return;
		m_iNumber = n;
		if (Replication.IsRunning())
			Replication.BumpMe();
		HmdServerRefreshPool();
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerCycleTextDir(int dir)
	{
		if (!m_bPlacedInWorld || m_bBeaconActive || m_fBatterySecondsRemaining <= 0.0)
			return;
		if (dir > 0)
		{
			m_iTextIndex++;
			if (m_iTextIndex >= TEXT_COUNT)
				m_iTextIndex = 0;
		}
		else
		{
			m_iTextIndex--;
			if (m_iTextIndex < 0)
				m_iTextIndex = TEXT_COUNT - 1;
		}
		if (Replication.IsRunning())
			Replication.BumpMe();
		HmdServerRefreshPool();
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerCycleNumberDir(int dir)
	{
		if (!m_bPlacedInWorld || m_bBeaconActive || m_fBatterySecondsRemaining <= 0.0)
			return;
		if (dir > 0)
		{
			m_iNumber++;
			if (m_iNumber > 7)
				m_iNumber = 1;
		}
		else
		{
			m_iNumber--;
			if (m_iNumber < 1)
				m_iNumber = 7;
		}
		if (Replication.IsRunning())
			Replication.BumpMe();
		HmdServerRefreshPool();
	}

	//------------------------------------------------------------------------------------------------
	void TryCycleTextDirection(int dir)
	{
		if (!Replication.IsRunning() || Replication.IsServer())
			ServerCycleTextDir(dir);
	}

	//------------------------------------------------------------------------------------------------
	void TryCycleNumberDirection(int dir)
	{
		if (!Replication.IsRunning() || Replication.IsServer())
			ServerCycleNumberDir(dir);
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerSetBeaconActive(bool active)
	{
		if (active)
		{
			if (m_fBatterySecondsRemaining <= 0.0)
				return;
			if (!m_bPlacedInWorld)
				return;
			m_bBeaconActive = true;
		}
		else
		{
			m_bBeaconActive = false;
		}
		if (Replication.IsRunning())
			Replication.BumpMe();
		HmdServerRefreshPool();
	}

	//------------------------------------------------------------------------------------------------
	void TrySetBeaconActive(bool active)
	{
		if (!Replication.IsRunning() || Replication.IsServer())
			ServerSetBeaconActive(active);
		else
			Rpc(RpcAsk_SetBeaconActive, active);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SetBeaconActive(bool active)
	{
		ServerSetBeaconActive(active);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!Replication.IsRunning() || Replication.IsServer())
		{
			float life = m_fBatteryLifeSeconds;
			if (life <= 0.0)
				life = BEACON_TOTAL_SECONDS;
			m_fBatterySecondsRemaining = life;
			m_bPlacedInWorld = false;
			m_bBeaconActive = false;
			if (m_iNumber < 1)
				m_iNumber = 1;
			if (m_iNumber > 7)
				m_iNumber = 7;
			if (m_iTextIndex < 0 || m_iTextIndex >= TEXT_COUNT)
				m_iTextIndex = 0;
			if (Replication.IsRunning())
				Replication.BumpMe();
			m_iDbgLastIffPoolPosLogMs = 0;
			HmdServerRefreshPool();
		}
		SetEventMask(owner, EntityEvent.FRAME | owner.GetEventMask());
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		if (!owner)
			return;
		if (!GetGame() || !GetGame().InPlayMode())
			return;

		if (!Replication.IsRunning() || Replication.IsServer())
		{
			AuthorityTickDeploymentGates(owner);

			if (m_bPlacedInWorld && m_bBeaconActive && m_fBatterySecondsRemaining > 0.0)
			{
				//! Game module: no `ChimeraWorld.Cast`; drain from authoritative frame delta (server / listen host).
				float dt = timeSlice;
				if (dt < 0.0)
					dt = 0.0;
				if (dt > BATTERY_DRAIN_DT_CAP)
					dt = BATTERY_DRAIN_DT_CAP;
				m_fBatterySecondsRemaining -= dt;
				if (m_fBatterySecondsRemaining <= 0.0)
				{
					m_fBatterySecondsRemaining = 0.0;
					m_bBeaconActive = false;
					m_fBatteryBumpAccum = 0.0;
					if (Replication.IsRunning())
						Replication.BumpMe();
					HmdServerRefreshPool();
					//! Drop pool row immediately; delete entity next tick (avoid deleting inside `EOnFrame` of same owner).
					if (GetGame())
						GetGame().GetCallqueue().CallLater(HmdServerDeleteOwnerDeferred, 0, false);
				}
				else if (Replication.IsRunning())
				{
					m_fBatteryBumpAccum += dt;
					if (m_fBatteryBumpAccum >= 1.0)
					{
						m_fBatteryBumpAccum = 0.0;
						Replication.BumpMe();
					}
				}
			}

			HmdServerRefreshPool();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdServerDeleteOwnerDeferred()
	{
		if (Replication.IsRunning() && !Replication.IsServer())
			return;
		IEntity ent = GetOwner();
		if (!ent)
			return;
		SCR_EntityHelper.DeleteEntityAndChildren(ent);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(HmdDeferredNotifyPlacedInWorld);
		if (Replication.IsServer())
		{
			IEntity src = owner;
			if (!src)
				src = GetOwner();
			if (src)
				HMD_GlobalHmdElementPoolComponent.StaticServerRemoveAllPoolRowsForSource(src);
			m_iPoolElementId = -1;
			m_sLastPoolText = string.Empty;
		}
		super.OnDelete(owner);
	}
}
