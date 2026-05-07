//------------------------------------------------------------------------------------------------
//! Shared server authority for HMD laser designation: code window, spawned `Laser_Designation.et`, replication, and trace-driven placement.
//! Vehicle-specific control (seats, helmet, turret bone) lives on **`HMD_LaserMarkingComponent`**. Handheld uses **`HMD_LaserMarkingHandheldComponent`** (extends this class only).
//! **Pool teardown:** `OnDelete` removes every global pool row whose `m_pSource` lies under `GetOwner()` (markers, designation anchors on children), then despawns the world designation entity.
[BaseContainerProps()]
class HMD_LaserMarkingCoreComponentClass : ScriptComponentClass
{
}

class HMD_LaserMarkingCoreComponent : ScriptComponent
{
	[Attribute("1111", UIWidgets.EditBox, "Inclusive minimum laser code", category: "HMD Laser code")]
	protected int m_iLaserCodeMin;

	[Attribute("1199", UIWidgets.EditBox, "Inclusive maximum laser code", category: "HMD Laser code")]
	protected int m_iLaserCodeMax;

	[Attribute("1111", UIWidgets.EditBox, "Initial laser code (clamped to min/max)", category: "HMD Laser code")]
	protected int m_iInitialLaserCode;

	[Attribute("{6900000900002001}Prefabs/HMD/Laser_Designation.et", UIWidgets.EditBox, "Server-spawned designation prefab at ray hit", category: "HMD Designation trace")]
	protected ResourceName m_rLaserDesignationPrefab;

	[Attribute("2000", UIWidgets.EditBox, "Max ray length (m) for designation trace", category: "HMD Designation trace")]
	protected float m_fDesignationTraceMaxRangeM;

	[Attribute("0", UIWidgets.CheckBox, "Draw designation trace with debug lines", category: "HMD Designation trace")]
	protected bool m_bDebugDrawDesignationRay;

	//! Server-authoritative; replicated. When false, no trace / spawn.
	[RplProp()]
	protected bool m_bLaserMarkingActive;

	//! Server-authoritative; replicated. Current laser display code; applied to spawned designation and pool when active.
	[RplProp()]
	protected int m_iVehicleLaserCode;

	protected IEntity m_pServerActiveLaserDesignationEnt;

	protected bool m_bHmdLaserCodeCacheValid;
	protected int m_iCachedLaserCodeMin;
	protected int m_iCachedLaserCodeMax;
	protected int m_iCachedVehicleLaserCode;

	//------------------------------------------------------------------------------------------------
	protected int HmdClampLaserCodeToPolicyWindow(int code)
	{
		int lo = m_iLaserCodeMin;
		int hi = m_iLaserCodeMax;
		if (lo > hi)
		{
			int t = lo;
			lo = hi;
			hi = t;
		}
		if (code < lo)
			return lo;
		if (code > hi)
			return hi;
		return code;
	}

	//------------------------------------------------------------------------------------------------
	//! Vehicle / handheld subclasses implement eligibility (seats, held gadget + viewport, etc.).
	bool HmdEvaluateLocalLaserControlEligibility(IEntity controlled)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	bool HmdIsLocalPlayerEligibleForLaserControl()
	{
		IEntity ch = SCR_PlayerController.GetLocalMainEntity();
		if (!ch)
			ch = SCR_PlayerController.GetLocalControlledEntity();
		return HmdEvaluateLocalLaserControlEligibility(ch);
	}

	//------------------------------------------------------------------------------------------------
	bool HmdIsLaserMarkingActive()
	{
		return m_bLaserMarkingActive;
	}

	//------------------------------------------------------------------------------------------------
	float HmdGetDesignationTraceMaxRangeM()
	{
		return m_fDesignationTraceMaxRangeM;
	}

	//------------------------------------------------------------------------------------------------
	int HmdGetReplicatedLaserCode()
	{
		return m_iVehicleLaserCode;
	}

	//------------------------------------------------------------------------------------------------
	//! Client HUD rangefinder: subclasses supply world ray matching server designation trace.
	bool HmdTryGetClientHudDesignationRay(out vector origin, out vector dir, out IEntity traceHost)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	HMD_DesignationBaseComponent HmdGetActiveLaserDesignationBase()
	{
		if (!m_pServerActiveLaserDesignationEnt)
			return null;
		HMD_DesignationElementComponent el = HMD_DesignationElementComponent.Cast(m_pServerActiveLaserDesignationEnt.FindComponent(HMD_DesignationElementComponent));
		if (el)
			return el;
		return HMD_DesignationBaseComponent.Cast(m_pServerActiveLaserDesignationEnt.FindComponent(HMD_DesignationBaseComponent));
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_bHmdLaserCodeCacheValid = false;
		if (Replication.IsServer())
		{
			m_iVehicleLaserCode = HmdClampLaserCodeToPolicyWindow(m_iInitialLaserCode);
			Replication.BumpMe();
		}
		if (Replication.IsServer() || m_bDebugDrawDesignationRay)
			SetEventMask(owner, EntityEvent.FRAME);
		if (!GetGame())
			return;
		HmdPushLaserCodePolicyToActiveDesignation();
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (Replication.IsServer())
		{
			IEntity mo = GetOwner();
			if (mo)
				HMD_GlobalHmdElementPoolComponent.StaticServerRemoveAllPoolRowsForSourcesInSubtree(mo);
		}
		HmdServerDespawnLaserDesignationIfAny();
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		if (Replication.IsServer())
		{
			int reclamped = HmdClampLaserCodeToPolicyWindow(m_iVehicleLaserCode);
			if (reclamped != m_iVehicleLaserCode)
			{
				m_iVehicleLaserCode = reclamped;
				Replication.BumpMe();
			}
			HmdServerUpdateLaserDesignationFromTrace(owner);
			if (m_bHmdLaserCodeCacheValid
				&& (m_iLaserCodeMin != m_iCachedLaserCodeMin || m_iLaserCodeMax != m_iCachedLaserCodeMax || m_iVehicleLaserCode != m_iCachedVehicleLaserCode))
				HmdPushLaserCodePolicyToActiveDesignation();
		}
		if (m_bDebugDrawDesignationRay && m_bLaserMarkingActive)
			HmdDebugDrawDesignationTraceRay(owner);
	}

	//------------------------------------------------------------------------------------------------
	void AskToggleLaserMarkingFromLocal()
	{
		if (!HmdIsLocalPlayerEligibleForLaserControl())
			return;
		IEntity owner = GetOwner();
		if (!owner)
			return;
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!rpl)
			return;
		if (rpl.IsMaster())
			RpcAsk_ToggleLaserMarkingActive();
		else
			Rpc(RpcAsk_ToggleLaserMarkingActive);
	}

	//------------------------------------------------------------------------------------------------
	void AskSetLaserMarkingActiveFromLocal(bool active)
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!rpl)
			return;
		if (rpl.IsMaster())
			RpcAsk_SetLaserMarkingActive(active);
		else
			Rpc(RpcAsk_SetLaserMarkingActive, active);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleLaserMarkingActive()
	{
		if (!Replication.IsServer())
			return;
		m_bLaserMarkingActive = !m_bLaserMarkingActive;
		if (!m_bLaserMarkingActive)
			HmdServerDespawnLaserDesignationIfAny();
		Replication.BumpMe();
		HMD_HmdDebug.SrvLaserMarking(string.Format("Toggle marking active -> %1", m_bLaserMarkingActive));
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SetLaserMarkingActive(bool active)
	{
		if (!Replication.IsServer())
			return;
		m_bLaserMarkingActive = active;
		if (!m_bLaserMarkingActive)
			HmdServerDespawnLaserDesignationIfAny();
		Replication.BumpMe();
		HMD_HmdDebug.SrvLaserMarking(string.Format("Set marking active -> %1", m_bLaserMarkingActive));
	}

	//------------------------------------------------------------------------------------------------
	void AskStepActiveDesignationLaserCodeFromLocal(int delta)
	{
		if (!HmdIsLocalPlayerEligibleForLaserControl())
			return;
		IEntity owner = GetOwner();
		if (!owner)
			return;
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!rpl)
			return;
		if (rpl.IsMaster())
			RpcAsk_StepActiveDesignationLaserCode(delta);
		else
			Rpc(RpcAsk_StepActiveDesignationLaserCode, delta);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_StepActiveDesignationLaserCode(int delta)
	{
		if (!Replication.IsServer())
			return;
		int next = HmdClampLaserCodeToPolicyWindow(m_iVehicleLaserCode + delta);
		if (next == m_iVehicleLaserCode)
			return;
		m_iVehicleLaserCode = next;
		Replication.BumpMe();
		HMD_HmdDebug.SrvLaserMarking(string.Format("Vehicle / core laser code -> %1", m_iVehicleLaserCode));
		HMD_DesignationBaseComponent des = HmdGetActiveLaserDesignationBase();
		if (des)
		{
			des.HmdApplyLaserCodePolicy(m_iLaserCodeMin, m_iLaserCodeMax, m_iVehicleLaserCode);
			des.HmdServerSyncPoolRowDisplayCode();
			m_iCachedLaserCodeMin = m_iLaserCodeMin;
			m_iCachedLaserCodeMax = m_iLaserCodeMax;
			m_iCachedVehicleLaserCode = m_iVehicleLaserCode;
			m_bHmdLaserCodeCacheValid = true;
		}
		else
		{
			m_bHmdLaserCodeCacheValid = false;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool HmdTryResolveDesignationTrace(IEntity markingOwner, out vector origin, out vector dir, out IEntity traceHost)
	{
		origin = vector.Zero;
		dir = vector.Zero;
		traceHost = null;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdServerUpdateLaserDesignationFromTrace(IEntity markingOwner)
	{
		if (!m_bLaserMarkingActive)
		{
			HmdServerDespawnLaserDesignationIfAny();
			return;
		}
		if (!markingOwner)
			return;
		vector origin;
		vector dir;
		IEntity traceHost;
		if (!HmdTryResolveDesignationTrace(markingOwner, origin, dir, traceHost))
		{
			HmdServerDespawnLaserDesignationIfAny();
			HMD_HmdDebug.SrvLaserMarkingThrottled("Designation trace: resolve failed (bone / camera ray / handheld ray TTL)", 750);
			return;
		}
		vector hit;
		float frac;
		if (!HMD_DesignatorRayTraceUtils.TraceRay(traceHost, origin, dir, m_fDesignationTraceMaxRangeM, hit, frac))
		{
			HmdServerDespawnLaserDesignationIfAny();
			HMD_HmdDebug.SrvLaserMarkingThrottled("Designation trace: no world hit within max range", 750);
			return;
		}
		HmdServerSpawnOrMoveLaserDesignationAt(markingOwner, hit);
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdServerSpawnOrMoveLaserDesignationAt(IEntity markingOwner, vector hitWorld)
	{
		if (!Replication.IsServer() || !markingOwner)
			return;
		if (m_rLaserDesignationPrefab.IsEmpty())
			return;
		BaseWorld world = markingOwner.GetWorld();
		if (!world)
			return;
		if (m_pServerActiveLaserDesignationEnt)
		{
			m_pServerActiveLaserDesignationEnt.SetOrigin(hitWorld);
			return;
		}
		Resource res = Resource.Load(m_rLaserDesignationPrefab);
		if (!res || !res.IsValid())
			return;
		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		vector wt[4];
		wt[0] = vector.Right;
		wt[1] = vector.Up;
		wt[2] = vector.Forward;
		wt[3] = hitWorld;
		sp.Transform = wt;
		IEntity spawned = GetGame().SpawnEntityPrefab(res, world, sp);
		if (!spawned)
			return;
		m_pServerActiveLaserDesignationEnt = spawned;
		m_bHmdLaserCodeCacheValid = false;
		HmdPushLaserCodePolicyToActiveDesignation();
		HMD_HmdDebug.SrvLaserMarking("Spawned server laser designation entity at trace hit");
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdServerDespawnLaserDesignationIfAny()
	{
		if (!Replication.IsServer())
			return;
		if (!m_pServerActiveLaserDesignationEnt)
			return;
		SCR_EntityHelper.DeleteEntityAndChildren(m_pServerActiveLaserDesignationEnt);
		m_pServerActiveLaserDesignationEnt = null;
		HMD_HmdDebug.SrvLaserMarking("Despawned server laser designation entity");
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdDebugDrawDesignationTraceRay(IEntity markingOwner)
	{
		if (!markingOwner)
			return;
		Game game = GetGame();
		if (!game || !game.InPlayMode())
			return;
		vector origin;
		vector dir;
		IEntity traceHost;
		if (!HmdTryResolveDesignationTrace(markingOwner, origin, dir, traceHost))
			return;
		vector hit;
		float frac;
		bool traced = HMD_DesignatorRayTraceUtils.TraceRay(traceHost, origin, dir, m_fDesignationTraceMaxRangeM, hit, frac);
		vector end = origin + dir * m_fDesignationTraceMaxRangeM;
		if (traced)
			end = hit;
		vector seg[2];
		seg[0] = origin;
		seg[1] = end;
		Shape.CreateLines(0xC0FFFF00, ShapeFlags.ONCE | ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP, seg, 2);
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdPushLaserCodePolicyToActiveDesignation()
	{
		if (!Replication.IsServer())
			return;
		HMD_DesignationBaseComponent des = HmdGetActiveLaserDesignationBase();
		if (!des)
			return;

		bool fullApply = !m_bHmdLaserCodeCacheValid || (m_iVehicleLaserCode != m_iCachedVehicleLaserCode);
		if (fullApply)
			des.HmdApplyLaserCodePolicy(m_iLaserCodeMin, m_iLaserCodeMax, m_iVehicleLaserCode);
		else
			des.HmdApplyLaserCodeBounds(m_iLaserCodeMin, m_iLaserCodeMax);
		des.HmdServerSyncPoolRowDisplayCode();

		m_iCachedLaserCodeMin = m_iLaserCodeMin;
		m_iCachedLaserCodeMax = m_iLaserCodeMax;
		m_iCachedVehicleLaserCode = m_iVehicleLaserCode;
		m_bHmdLaserCodeCacheValid = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Prefer **held** designator (zoomed) over vehicle-mounted marking so the correct operator path wins.
	protected static HMD_LaserMarkingCoreComponent HmdResolveLocalHudLaserMarking(IEntity pawn)
	{
		if (!pawn)
			return null;
		SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(pawn);
		if (gm)
		{
			SCR_GadgetComponent held = gm.GetHeldGadgetComponent();
			if (held && held.GetOwner())
			{
				HMD_LaserMarkingCoreComponent lmHand = HMD_EntityHmdHelpers.FindFirstLaserMarkingInHierarchy(held.GetOwner(), 8);
				if (lmHand && lmHand.HmdIsLaserMarkingActive() && lmHand.HmdIsLocalPlayerEligibleForLaserControl())
					return lmHand;
			}
		}
		IEntity root = pawn.GetRootParent();
		if (root && root != pawn)
		{
			HMD_LaserMarkingCoreComponent lmVeh = HMD_EntityHmdHelpers.FindFirstLaserMarkingInHierarchy(root, 12);
			if (lmVeh && lmVeh.HmdIsLaserMarkingActive() && lmVeh.HmdIsLocalPlayerEligibleForLaserControl())
				return lmVeh;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Client HUD: world hit from **this frame's** local designation ray (not replicated pool position), when the row is **our** active LASE.
	static bool StaticTryGetLocalHudDesignationWorldHitForRow(HMD_PoolMirrorRow row, out vector hitWorld)
	{
		hitWorld = vector.Zero;
		if (!row)
			return false;
		if (row.m_eKind != EHmdElementKind.TARGET_DESIGNATION)
			return false;
		if (!GetGame() || !GetGame().GetPlayerController())
			return false;
		IEntity pawn = SCR_PlayerController.GetLocalMainEntity();
		if (!pawn)
			pawn = SCR_PlayerController.GetLocalControlledEntity();
		if (!pawn)
			return false;
		HMD_LaserMarkingCoreComponent lm = HmdResolveLocalHudLaserMarking(pawn);
		if (!lm)
			return false;
		if (row.m_iCode != lm.HmdGetReplicatedLaserCode())
			return false;
		vector o;
		vector d;
		IEntity traceHost;
		if (!lm.HmdTryGetClientHudDesignationRay(o, d, traceHost))
			return false;
		float frac;
		if (!HMD_DesignatorRayTraceUtils.TraceRay(traceHost, o, d, lm.HmdGetDesignationTraceMaxRangeM(), hitWorld, frac))
			return false;
		return true;
	}
}
