//------------------------------------------------------------------------------------------------
//! World designation anchor: registers `EHmdElementKind.TARGET_DESIGNATION` in `HMD_GlobalHmdElementPoolComponent`
//! and updates pool world position from the owner entity each server frame (same pattern as `HMD_MarkerElementBaseComponent`).
//! Target class, label text, and laser code are supplied by `HMD_DesignationElementComponent` or `HmdApplyLaserCodePolicy` from `HMD_LaserMarkingComponent`.
//! **Pool lifecycle:** `OnDelete` calls `StaticServerRemoveAllPoolRowsForSource` for `GetOwner()`.
class HMD_DesignationBaseComponentClass : ScriptComponentClass
{
}

class HMD_DesignationBaseComponent : ScriptComponent
{
	protected int m_iLaserCodeMin = 1111;

	protected int m_iLaserCodeMax = 1199;

	//! Set by `HMD_DesignationElementComponent` before pool registration, or by policy from `HMD_LaserMarkingComponent`.
	protected string m_sHmdPoolLabel = "LASE";

	//! 0 Lock, 1 Generic, 3 IFF-style (HUD shows pool label only — see `HMD_ElementHudBridge`).
	protected int m_iHmdDesignationClass = 1;

	[RplProp(onRplName: "OnRpl_DisplayLaserCode")]
	protected int m_iDisplayLaserCode = 1111;

	protected int m_iPoolElementId = -1;

	//! Passed to `ServerRegisterElement`: when true, HUD may draw this row while global designation visibility is off, if local vehicle laser marking is active.
	protected bool m_bPoolRowVehicleLaserHudBypass;

	//------------------------------------------------------------------------------------------------
	protected void OnRpl_DisplayLaserCode()
	{
	}

	//------------------------------------------------------------------------------------------------
	//! Used by `HMD_DesignationElementComponent` before `super.OnPostInit` so the first pool row uses the correct metadata.
	protected void HmdSetDesignationMetadata(int designationClass, string poolLabel)
	{
		m_iHmdDesignationClass = designationClass;
		if (!poolLabel.IsEmpty())
			m_sHmdPoolLabel = poolLabel;
	}

	//------------------------------------------------------------------------------------------------
	//! Set before `OnPostInit` / pool registration (used by `HMD_DesignationElementComponent` for vehicle-spawned designations).
	protected void HmdSetPoolRowVehicleLaserHudBypass(bool bypass)
	{
		m_bPoolRowVehicleLaserHudBypass = bypass;
	}

	//------------------------------------------------------------------------------------------------
	void HmdApplyLaserCodePolicy(int codeMin, int codeMax, int initialCode)
	{
		int lo = codeMin;
		int hi = codeMax;
		if (lo > hi)
		{
			int t = lo;
			lo = hi;
			hi = t;
		}
		m_iLaserCodeMin = lo;
		m_iLaserCodeMax = hi;
		int next = initialCode;
		if (next < m_iLaserCodeMin)
			next = m_iLaserCodeMin;
		if (next > m_iLaserCodeMax)
			next = m_iLaserCodeMax;
		m_iDisplayLaserCode = next;
		if (Replication.IsServer())
			Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void HmdApplyLaserCodeBounds(int codeMin, int codeMax)
	{
		int lo = codeMin;
		int hi = codeMax;
		if (lo > hi)
		{
			int t = lo;
			lo = hi;
			hi = t;
		}
		m_iLaserCodeMin = lo;
		m_iLaserCodeMax = hi;
		HmdNormalizeDisplayLaserCode();
		if (Replication.IsServer())
			Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void HmdServerSyncPoolRowDisplayCode()
	{
		if (!Replication.IsServer())
			return;
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (pool && m_iPoolElementId >= 0)
			pool.ServerUpdateElementCode(m_iPoolElementId, m_iDisplayLaserCode);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		HmdNormalizeDisplayLaserCode();
		SetEventMask(owner, EntityEvent.FRAME);
		//! Do **not** call `HmdRegisterOnce` here: vehicle `HMD_LaserMarkingComponent` applies `m_iVehicleLaserCode` to this component immediately after spawning this entity; the first `EOnFrame` registers the pool row with the correct `m_iDisplayLaserCode`.
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdNormalizeDisplayLaserCode()
	{
		if (m_iLaserCodeMin > m_iLaserCodeMax)
		{
			int t = m_iLaserCodeMin;
			m_iLaserCodeMin = m_iLaserCodeMax;
			m_iLaserCodeMax = t;
		}
		if (m_iDisplayLaserCode < m_iLaserCodeMin || m_iDisplayLaserCode > m_iLaserCodeMax)
			m_iDisplayLaserCode = m_iLaserCodeMin;
	}

	//------------------------------------------------------------------------------------------------
	int GetDisplayLaserCode()
	{
		return m_iDisplayLaserCode;
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		if (!Replication.IsServer())
			return;
		if (!owner)
		{
			if (m_iPoolElementId >= 0)
				HmdUnregisterFromPoolByIdOnly();
			return;
		}
		if (m_iPoolElementId < 0)
			HmdRegisterOnce();
		if (m_iPoolElementId < 0)
			return;
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (!pool)
			return;
		pool.ServerSetElementWorldPosition(m_iPoolElementId, owner.GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdRegisterOnce()
	{
		if (!Replication.IsServer())
			return;
		if (m_iPoolElementId >= 0)
			return;
		IEntity owner = GetOwner();
		if (!owner)
			return;
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (!pool)
		{
			SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
			if (gm)
				HMD_GlobalHmdElementPoolBootstrap.ServerEnsurePoolExists(gm);
			pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		}
		if (!pool)
			return;
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
		m_iPoolElementId = pool.ServerRegisterElement(owner, EHmdElementKind.TARGET_DESIGNATION, m_iHmdDesignationClass, m_sHmdPoolLabel, m_iDisplayLaserCode, p0, p1, m_bPoolRowVehicleLaserHudBypass, 0xFFFFFFFF);
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdUnregisterFromPoolByIdOnly()
	{
		if (!Replication.IsServer() || m_iPoolElementId < 0)
			return;
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (pool)
			pool.ServerRemoveElement(m_iPoolElementId);
		m_iPoolElementId = -1;
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (Replication.IsServer())
		{
			IEntity src = owner;
			if (!src)
				src = GetOwner();
			if (src)
				HMD_GlobalHmdElementPoolComponent.StaticServerRemoveAllPoolRowsForSource(src);
			else if (m_iPoolElementId >= 0)
			{
				HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
				if (pool)
					pool.ServerRemoveElement(m_iPoolElementId);
			}
			m_iPoolElementId = -1;
		}
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_StepLaserCode(int delta)
	{
		int next = m_iDisplayLaserCode + delta;
		if (next < m_iLaserCodeMin)
			next = m_iLaserCodeMin;
		if (next > m_iLaserCodeMax)
			next = m_iLaserCodeMax;
		if (next == m_iDisplayLaserCode)
			return;
		m_iDisplayLaserCode = next;
		Replication.BumpMe();
		HmdServerSyncPoolRowDisplayCode();
	}

	//------------------------------------------------------------------------------------------------
	void AskStepLaserCodeFromOwner(int delta)
	{
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (!rpl)
			return;
		if (rpl.IsMaster())
			RpcAsk_StepLaserCode(delta);
		else
			Rpc(RpcAsk_StepLaserCode, delta);
	}
}
