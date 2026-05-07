//------------------------------------------------------------------------------------------------
//! WCS handheld laser designator subclass: syncs designated world point into HMD_GlobalHmdElementPoolComponent
//! and exposes configurable laser code bounds with server-authoritative stepping.
class HMD_DesignationElementBaseComponentClass : WCS_Armament_HandheldLaserDesignatorComponentClass
{
}

class HMD_DesignationElementBaseComponent : WCS_Armament_HandheldLaserDesignatorComponent
{
	//! Inclusive bounds; set by `HMD_LaserMarkingComponent` (vehicle) or `HMD_DesignationGeneratorComponent` (handheld), not prefab attributes on this class.
	protected int m_iLaserCodeMin = 1111;

	protected int m_iLaserCodeMax = 1199;

	[Attribute("LASE", UIWidgets.EditBox, "Pool row label text", category: "HMD")]
	protected string m_sHmdPoolLabel;

	[Attribute("1", UIWidgets.EditBox, "Target designation class: 0 Lock, 1 Generic, 3 IFF-style (pool HUD shows tag only)", category: "HMD")]
	protected int m_iHmdDesignationClass;

	//! Replicated for HUD; clamped on server in RpcAsk_StepLaserCode. Bounds from `HmdApplyLaserCodePolicy` (marking / generator).
	[RplProp(onRplName: "OnRpl_DisplayLaserCode")]
	protected int m_iDisplayLaserCode;

	protected int m_iPoolElementId = -1;

	//------------------------------------------------------------------------------------------------
	protected void OnRpl_DisplayLaserCode()
	{
	}

	//------------------------------------------------------------------------------------------------
	//! Optional prefab sibling `HMD_DesignationGeneratorComponent` applies before WCS init (handheld / label + class only).
	void HmdApplyGeneratorOverrides(int designationClass, string poolLabel)
	{
		m_iHmdDesignationClass = designationClass;
		if (!poolLabel.IsEmpty())
			m_sHmdPoolLabel = poolLabel;
	}

	//------------------------------------------------------------------------------------------------
	//! Laser code window and initial value: applied from `HMD_LaserMarkingComponent` (vehicle) or `HMD_DesignationGeneratorComponent` (handheld).
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
	//! Updates min/max only and reclamps `m_iDisplayLaserCode` (live edit of bounds without forcing `initialCode`).
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
	//! Server: push replicated display code into the pool row when one exists (`HMD_LaserMarkingComponent` policy refresh, etc.).
	void HmdServerSyncPoolRowDisplayCode()
	{
		if (!Replication.IsServer())
			return;
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (pool && m_iPoolElementId >= 0)
			pool.ServerUpdateElementCode(m_iPoolElementId, m_iDisplayLaserCode);
	}

	//------------------------------------------------------------------------------------------------
	//! Server-only: push a traced world hit into WCS designation (pool row updates via SetDesignationPosition).
	void HmdServerRefreshDesignatedPosition(vector worldPos)
	{
		if (!Replication.IsServer())
			return;
		if (!IsDesignating())
			return;
		SetDesignationPosition(worldPos);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		HMD_DesignationGeneratorComponent gen = HMD_DesignationGeneratorComponent.Cast(owner.FindComponent(HMD_DesignationGeneratorComponent));
		if (gen)
			gen.PushToDesignation(this);
		super.OnPostInit(owner);
		HmdNormalizeDisplayLaserCode();
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
	protected void ResolveParentRplIds(out RplId parent0, out RplId parent1)
	{
		parent0 = RplId.Invalid();
		parent1 = RplId.Invalid();
		IEntity owner = GetOwner();
		if (!owner)
			return;
		IEntity p = owner.GetParent();
		if (p)
		{
			RplComponent r0 = RplComponent.Cast(p.FindComponent(RplComponent));
			if (r0)
				parent0 = Replication.FindId(r0);
			IEntity p2 = p.GetParent();
			if (p2)
			{
				RplComponent r1 = RplComponent.Cast(p2.FindComponent(RplComponent));
				if (r1)
					parent1 = Replication.FindId(r1);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdEnsurePoolRow()
	{
		if (!Replication.IsServer())
			return;
		if (m_iPoolElementId >= 0)
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
		RplId p0;
		RplId p1;
		ResolveParentRplIds(p0, p1);
		m_iPoolElementId = pool.ServerRegisterElement(GetOwner(), EHmdElementKind.TARGET_DESIGNATION, m_iHmdDesignationClass, m_sHmdPoolLabel, m_iDisplayLaserCode, p0, p1, false, 0xFFFFFFFF);
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdRemovePoolRow()
	{
		if (!Replication.IsServer())
			return;
		IEntity src = GetOwner();
		if (!src)
		{
			if (m_iPoolElementId >= 0)
			{
				HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
				if (pool)
					pool.ServerRemoveElement(m_iPoolElementId);
			}
			m_iPoolElementId = -1;
			return;
		}
		HMD_GlobalHmdElementPoolComponent.StaticServerRemoveAllPoolRowsForSource(src);
		m_iPoolElementId = -1;
	}

	//------------------------------------------------------------------------------------------------
	protected override void SetDesignationPosition(vector position)
	{
		super.SetDesignationPosition(position);
		if (!Replication.IsServer())
			return;
		//! Pool row must exist while designating even before WCS reports `HasValidDesignation` (vehicle traces can lag a frame; markers retry pool in `EOnFrame`).
		if (!IsDesignating())
			return;
		HmdEnsurePoolRow();
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (pool && m_iPoolElementId >= 0 && HasValidDesignation())
			pool.ServerSetElementWorldPosition(m_iPoolElementId, position);
	}

	//------------------------------------------------------------------------------------------------
	protected override void SetDesignatingState(bool state)
	{
		super.SetDesignatingState(state);
		if (!Replication.IsServer())
			return;
		HMD_HmdDebug.SrvLaserMarking(string.Format("Handheld WCS designating state=%1 poolRowId=%2", state, m_iPoolElementId));
		if (!state)
		{
			HmdRemovePoolRow();
			return;
		}
		//! Register immediately so the global pool lists the designation before the first valid hit / `SetDesignationPosition`.
		HmdEnsurePoolRow();
	}

	//------------------------------------------------------------------------------------------------
	override void RpcAsk_SetDesignationInvalid()
	{
		super.RpcAsk_SetDesignationInvalid();
		if (!Replication.IsServer())
			return;
		HmdRemovePoolRow();
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		HmdRemovePoolRow();
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
		HMD_HmdDebug.SrvLaserMarking(string.Format("Handheld laser code (WCS) -> %1", m_iDisplayLaserCode));
		HmdServerSyncPoolRowDisplayCode();
	}

	//------------------------------------------------------------------------------------------------
	//! Called from local input when the designator gadget is authoritative to the player.
	void AskStepLaserCodeFromOwner(int delta)
	{
		if (IsMaster())
			RpcAsk_StepLaserCode(delta);
		else
			Rpc(RpcAsk_StepLaserCode, delta);
	}

	//------------------------------------------------------------------------------------------------
	//! Same replication pattern as WCS_Armament_HandheldLaserDesignatorComponent.OnDesignateToggle (HMD_LaserDesignate / T).
	void HmdToggleDesignatingForInput()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!rpl)
			return;

		bool newState = !m_bIsDesignating;
		m_bIsDesignating = newState;

		if (rpl.IsMaster())
			SetDesignatingState(newState);
		else
			Rpc(RpcAsk_SetDesignating, newState);

		if (newState)
			ActivateDesignation();
		else
			DeactivateDesignation();
	}
}
