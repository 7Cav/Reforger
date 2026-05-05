//------------------------------------------------------------------------------------------------
//! Handheld laser designator gadget: extends **`HMD_LaserMarkingCoreComponent`** only (no vehicle seat / helmet / bone options).
//! Eligibility: local character **holds** this gadget and is **looking through the designator viewport** (`HMD_HandheldDesignatorOpticZoom.IsZoomedForHMD`, same idea as LaserFixesAgain).
//! Trace: **active world camera**; Rpc to server on network clients, or **ApplyHandheldCameraRayOnAuthority** when `IsServer && !IsClient` (Workbench host).
[BaseContainerProps()]
class HMD_LaserMarkingHandheldComponentClass : HMD_LaserMarkingCoreComponentClass
{
}

class HMD_LaserMarkingHandheldComponent : HMD_LaserMarkingCoreComponent
{
	//! Server: last camera ray pushed from the owning client (milliseconds from `System.GetTickCount`).
	protected int m_iSrvHandheldCameraRayTickMs;

	protected vector m_vSrvHandheldCameraRayOrigin;

	protected vector m_vSrvHandheldCameraRayDir;

	protected int m_iCliLastHandheldRaySendMs;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		//! Core enables `FRAME` on server (or client when debug draw). Handheld needs **client** frames to push camera LOS to authority.
		SetEventMask(owner, EntityEvent.FRAME | owner.GetEventMask());
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_ChimeraCharacter HmdResolveCarrierCharacterForGadget(IEntity gadget)
	{
		if (!gadget)
			return null;
		IEntity w = gadget;
		int guard = 0;
		while (w && guard++ < 32)
		{
			SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(w);
			if (ch)
				return ch;
			w = w.GetParent();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	override bool HmdEvaluateLocalLaserControlEligibility(IEntity controlled)
	{
		if (!controlled)
			return false;
		IEntity gadget = GetOwner();
		if (!gadget)
			return false;
		SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(controlled);
		if (!gm)
			return false;
		SCR_GadgetComponent held = gm.GetHeldGadgetComponent();
		if (!held || held.GetOwner() != gadget)
			return false;
		return HMD_HandheldDesignatorOpticZoom.IsZoomedForHMD();
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		if (!owner)
			return;
		//! Do **not** require `Replication.IsClient()`: Workbench / editor host often has local player but `cli=0` in HUD handshake.
		IEntity local = SCR_PlayerController.GetLocalMainEntity();
		if (!local)
			local = SCR_PlayerController.GetLocalControlledEntity();
		if (!local)
			return;
		SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(local);
		if (!gm)
			return;
		SCR_GadgetComponent held = gm.GetHeldGadgetComponent();
		if (!held || held.GetOwner() != owner)
			return;
		if (!HMD_HandheldDesignatorOpticZoom.IsZoomedForHMD())
			return;
		vector o;
		vector d;
		if (!HMD_DesignatorRayTraceUtils.TryGetLocalPlayerCameraWorldRay(o, d))
			return;
		int now = System.GetTickCount();
		if (now - m_iCliLastHandheldRaySendMs < 12)
			return;
		m_iCliLastHandheldRaySendMs = now;
		if (Replication.IsServer() && !Replication.IsClient())
		{
			ApplyHandheldCameraRayOnAuthority(o, d);
		}
		else
		{
			RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
			if (!rpl)
				return;
			Rpc(RpcPushHandheldCameraRayFromLocal, o, d);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Server authority: store last camera ray for `HmdTryResolveDesignationTrace` (Rpc from client or direct call from local host).
	protected void ApplyHandheldCameraRayOnAuthority(vector origin, vector dirRaw)
	{
		if (!Replication.IsServer())
			return;
		IEntity gadget = GetOwner();
		if (!gadget)
			return;
		SCR_ChimeraCharacter ch = HmdResolveCarrierCharacterForGadget(gadget);
		if (!ch)
			return;
		vector chPos = ch.GetOrigin();
		if ((origin - chPos).Length() > 14.0)
			return;
		float dl = dirRaw.Length();
		if (dl < 0.001)
			return;
		m_vSrvHandheldCameraRayOrigin = origin;
		m_vSrvHandheldCameraRayDir = dirRaw * (1.0 / dl);
		m_iSrvHandheldCameraRayTickMs = System.GetTickCount();
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcPushHandheldCameraRayFromLocal(vector origin, vector dirRaw)
	{
		ApplyHandheldCameraRayOnAuthority(origin, dirRaw);
	}

	//------------------------------------------------------------------------------------------------
	override protected bool HmdTryResolveDesignationTrace(IEntity markingOwner, out vector origin, out vector dir, out IEntity traceHost)
	{
		origin = vector.Zero;
		dir = vector.Zero;
		traceHost = null;
		if (!markingOwner)
			return false;
		if (!Replication.IsServer())
		{
			HMD_DesignatorRayTraceUtils.TryGetLocalPlayerCameraWorldRay(origin, dir);
			traceHost = markingOwner;
			return dir.LengthSq() > 0.0001;
		}
		const int ttlMs = 450;
		int age = System.GetTickCount() - m_iSrvHandheldCameraRayTickMs;
		if (m_iSrvHandheldCameraRayTickMs != 0 && age >= 0 && age <= ttlMs && m_vSrvHandheldCameraRayDir.LengthSq() > 0.0001)
		{
			origin = m_vSrvHandheldCameraRayOrigin;
			dir = m_vSrvHandheldCameraRayDir;
			SCR_ChimeraCharacter ch = HmdResolveCarrierCharacterForGadget(markingOwner);
			traceHost = ch;
			if (!traceHost)
				traceHost = markingOwner;
			return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool HmdTryGetClientHudDesignationRay(out vector origin, out vector dir, out IEntity traceHost)
	{
		origin = vector.Zero;
		dir = vector.Zero;
		traceHost = null;
		IEntity markingOwner = GetOwner();
		if (!markingOwner)
			return false;
		if (!HMD_DesignatorRayTraceUtils.TryGetLocalPlayerCameraWorldRay(origin, dir))
			return false;
		SCR_ChimeraCharacter ch = HmdResolveCarrierCharacterForGadget(markingOwner);
		traceHost = ch;
		if (!traceHost)
			traceHost = markingOwner;
		return dir.LengthSq() > 0.0001;
	}
}
