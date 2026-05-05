//------------------------------------------------------------------------------------------------
//! Vehicle laser marking when the **marking component may live on a child entity** (turret / FLIR) but
//! crew in **hull compartments** still operate it (AH-6 / AH-64 style). Uses `HMD_ElementVisualizationVehicleComponent`
//! on the driveable root when `GetOwner() != GetRootParent()`; otherwise falls back to configured seat tokens on the hull.
//!
//! **Laser Reference mode (numpad 6):** disables vehicle laser marking and repurposes numpad `/` (RP), `8` (WP), `7` (delete scoped marker).
[BaseContainerProps()]
class HMD_LaserMarkingRemoteVehicleComponentClass : HMD_LaserMarkingComponentClass
{
}

class HMD_LaserMarkingRemoteVehicleComponent : HMD_LaserMarkingComponent
{
	[Attribute("{6900000A00000001}Prefabs/HMD/HMD_ReferenceMarker.et", UIWidgets.EditBox, "Replicated marker prefab for Laser Reference mode (defer-register marker).", category: "HMD Laser Reference")]
	protected ResourceName m_rReferenceMarkerPrefab;

	[RplProp()]
	protected bool m_bLaserReferenceMode;

	protected int m_iNextWpIndex = 1;
	protected int m_iNextRpIndex = 1;

	protected IEntity m_pRefDelBestEnt;
	protected float m_fRefDelBestDsq;

	//------------------------------------------------------------------------------------------------
	bool HmdIsLaserReferenceMode()
	{
		return m_bLaserReferenceMode;
	}

	//------------------------------------------------------------------------------------------------
	override bool HmdEvaluateLocalLaserControlEligibility(IEntity controlled)
	{
		if (!controlled)
			return false;
		IEntity markingEnt = GetOwner();
		if (!markingEnt)
			return false;
		if (controlled.GetRootParent() != markingEnt.GetRootParent())
			return false;
		IEntity vehicleRoot = markingEnt.GetRootParent();
		if (!vehicleRoot)
			vehicleRoot = markingEnt;
		if (markingEnt != vehicleRoot)
		{
			//! Turret-mounted marking: do **not** treat general vehicle-HUD seats as laser operators when
			//! `m_sLaserControlSeatNames` is set — hull/pilot HUD eligibility must not bypass gunner-only (etc.) tokens.
			if (!m_aLaserControlSeatTokens.IsEmpty())
				return HmdEvaluateLaserControlByConfiguredSeatTokens(controlled);
			HMD_ElementVisualizationVehicleComponent pol = HMD_ElementVisualizationVehicleComponent.Cast(HMD_EntityHmdHelpers.FindComponentInHierarchy(vehicleRoot, HMD_ElementVisualizationVehicleComponent));
			if (pol && pol.HmdEvaluateLocalVehicleHudEligibility(controlled))
				return HmdPassesHelmetForLaserControl(controlled);
		}
		return HmdEvaluateLaserControlByConfiguredSeatTokens(controlled);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		if (Replication.IsServer() && m_bLaserReferenceMode && m_bLaserMarkingActive)
		{
			m_bLaserMarkingActive = false;
			HmdServerDespawnLaserDesignationIfAny();
			Replication.BumpMe();
			HMD_HmdDebug.SrvLaserRef("Reference mode: forced laser marking off (server guard)");
		}
	}

	//------------------------------------------------------------------------------------------------
	void AskSetLaserReferenceModeFromLocal(bool wantOn)
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
			RpcAsk_SetLaserReferenceMode(wantOn);
		else
			Rpc(RpcAsk_SetLaserReferenceMode, wantOn);
	}

	//------------------------------------------------------------------------------------------------
	void AskToggleLaserReferenceModeFromLocal()
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
			RpcAsk_ToggleLaserReferenceMode();
		else
			Rpc(RpcAsk_ToggleLaserReferenceMode);
	}

	//------------------------------------------------------------------------------------------------
	void AskPlaceWpMarkerFromLocal()
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
			RpcAsk_PlaceWpMarker();
		else
			Rpc(RpcAsk_PlaceWpMarker);
	}

	//------------------------------------------------------------------------------------------------
	void AskPlaceRpMarkerFromLocal()
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
			RpcAsk_PlaceRpMarker();
		else
			Rpc(RpcAsk_PlaceRpMarker);
	}

	//------------------------------------------------------------------------------------------------
	void AskDeleteScopedMarkerFromLocal()
	{
		if (!HmdIsLocalPlayerEligibleForLaserControl())
			return;
		vector o;
		vector d;
		if (!HMD_DesignatorRayTraceUtils.TryGetLocalPlayerCameraWorldRay(o, d))
		{
			HMD_HmdDebug.CliLaserRef("Delete marker: no camera ray");
			return;
		}
		IEntity owner = GetOwner();
		if (!owner)
			return;
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!rpl)
			return;
		if (rpl.IsMaster())
			RpcAsk_DeleteScopedMarker(o, d);
		else
			Rpc(RpcAsk_DeleteScopedMarker, o, d);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SetLaserReferenceMode(bool wantOn)
	{
		if (!Replication.IsServer())
			return;
		if (m_bLaserReferenceMode == wantOn)
			return;
		m_bLaserReferenceMode = wantOn;
		if (m_bLaserReferenceMode)
		{
			m_bLaserMarkingActive = false;
			HmdServerDespawnLaserDesignationIfAny();
			HMD_HmdDebug.SrvLaserRef("Reference mode ON; laser marking forced off");
		}
		else
		{
			HMD_HmdDebug.SrvLaserRef("Reference mode OFF");
		}
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleLaserReferenceMode()
	{
		if (!Replication.IsServer())
			return;
		RpcAsk_SetLaserReferenceMode(!m_bLaserReferenceMode);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_PlaceWpMarker()
	{
		if (!Replication.IsServer())
			return;
		if (!m_bLaserReferenceMode)
		{
			HMD_HmdDebug.SrvLaserRef("PlaceWp rejected: not in reference mode");
			return;
		}
		HmdServerSpawnReferenceMarker(1);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_PlaceRpMarker()
	{
		if (!Replication.IsServer())
			return;
		if (!m_bLaserReferenceMode)
		{
			HMD_HmdDebug.SrvLaserRef("PlaceRp rejected: not in reference mode");
			return;
		}
		HmdServerSpawnReferenceMarker(2);
	}

	//------------------------------------------------------------------------------------------------
	//! `informationalClass` 1 = Waypoint (WP#), 2 = Reference (RP#).
	protected void HmdServerSpawnReferenceMarker(int informationalClass)
	{
		IEntity markingOwner = GetOwner();
		if (!markingOwner)
			return;
		vector origin;
		vector dir;
		IEntity traceHost;
		if (!HmdTryResolveDesignationTrace(markingOwner, origin, dir, traceHost))
		{
			HMD_HmdDebug.SrvLaserRef("Spawn reference marker: trace resolve failed");
			return;
		}
		vector hit;
		float frac;
		if (!HMD_DesignatorRayTraceUtils.TraceRay(traceHost, origin, dir, m_fDesignationTraceMaxRangeM, hit, frac))
		{
			HMD_HmdDebug.SrvLaserRef("Spawn reference marker: no hit");
			return;
		}
		if (m_rReferenceMarkerPrefab.IsEmpty())
		{
			HMD_HmdDebug.SrvLaserRef("Spawn reference marker: prefab path empty");
			return;
		}
		Resource res = Resource.Load(m_rReferenceMarkerPrefab);
		if (!res || !res.IsValid())
		{
			HMD_HmdDebug.SrvLaserRef("Spawn reference marker: resource invalid");
			return;
		}
		BaseWorld world = markingOwner.GetWorld();
		if (!world)
			return;
		int idx = 1;
		string prefix = "WP";
		if (informationalClass == 1)
		{
			idx = m_iNextWpIndex;
			m_iNextWpIndex++;
			prefix = "WP";
		}
		else if (informationalClass == 2)
		{
			idx = m_iNextRpIndex;
			m_iNextRpIndex++;
			prefix = "RP";
		}
		string label = string.Format("%1%2", prefix, idx);
		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		vector wt[4];
		wt[0] = vector.Right;
		wt[1] = vector.Up;
		wt[2] = vector.Forward;
		wt[3] = hit;
		sp.Transform = wt;
		IEntity spawned = GetGame().SpawnEntityPrefab(res, world, sp);
		if (!spawned)
		{
			HMD_HmdDebug.SrvLaserRef("Spawn reference marker: SpawnEntityPrefab failed");
			return;
		}
		IEntity attachParent = HmdResolveReferenceMarkerAttachParent(markingOwner, traceHost);
		if (!attachParent)
			attachParent = markingOwner;
		attachParent.AddChild(spawned, -1);
		spawned.SetOrigin(hit);
		HMD_MarkerElementBaseComponent m = HMD_MarkerElementBaseComponent.Cast(spawned.FindComponent(HMD_MarkerElementBaseComponent));
		if (!m)
		{
			HMD_HmdDebug.SrvLaserRef("Spawn reference marker: missing HMD_MarkerElementBaseComponent on prefab");
			SCR_EntityHelper.DeleteEntityAndChildren(spawned);
			return;
		}
		m.HmdServerFinalizePoolRegistration(informationalClass, label);
		HMD_HmdDebug.SrvLaserRef(string.Format("Spawn reference marker: registered %1 class=%2", label, informationalClass));
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_DeleteScopedMarker(vector rayOrigin, vector rayDirRaw)
	{
		if (!Replication.IsServer())
			return;
		if (!m_bLaserReferenceMode)
		{
			HMD_HmdDebug.SrvLaserRef("Delete marker rejected: not in reference mode");
			return;
		}
		IEntity markingRoot = GetOwner();
		if (!markingRoot)
			return;
		float dl = rayDirRaw.Length();
		if (dl < 0.001)
		{
			HMD_HmdDebug.SrvLaserRef("Delete marker: invalid ray dir");
			return;
		}
		vector dir = rayDirRaw * (1.0 / dl);
		vector hit;
		float frac;
		if (!HMD_DesignatorRayTraceUtils.TraceRay(markingRoot, rayOrigin, dir, m_fDesignationTraceMaxRangeM, hit, frac))
		{
			HMD_HmdDebug.SrvLaserRef("Delete marker: trace miss");
			return;
		}
		const float maxPickM = 10.0;
		m_pRefDelBestEnt = null;
		m_fRefDelBestDsq = 999999999.0;
		IEntity ch = markingRoot.GetChildren();
		while (ch)
		{
			HmdConsiderMarkerEntityForDelete(ch, markingRoot, hit, maxPickM);
			ch = ch.GetSibling();
		}
		if (!m_pRefDelBestEnt)
		{
			HMD_HmdDebug.SrvLaserRef("Delete marker: no scoped candidate within 10m of hit");
			return;
		}
		SCR_EntityHelper.DeleteEntityAndChildren(m_pRefDelBestEnt);
		HMD_HmdDebug.SrvLaserRef("Delete marker: removed entity");
	}

	//------------------------------------------------------------------------------------------------
	protected void HmdConsiderMarkerEntityForDelete(IEntity ent, IEntity markingRoot, vector hitWorld, float maxDistM)
	{
		if (!ent)
			return;
		HMD_MarkerElementBaseComponent mk = HMD_MarkerElementBaseComponent.Cast(ent.FindComponent(HMD_MarkerElementBaseComponent));
		if (mk && HmdEntityIsUnderMarkingHierarchy(ent, markingRoot))
		{
			int cls = mk.HmdGetInformationalClass();
			if (cls == 1 || cls == 2)
			{
				vector d = ent.GetOrigin() - hitWorld;
				float dsq = d.LengthSq();
				if (dsq <= maxDistM * maxDistM && dsq < m_fRefDelBestDsq)
				{
					m_pRefDelBestEnt = ent;
					m_fRefDelBestDsq = dsq;
				}
			}
		}
		IEntity c = ent.GetChildren();
		while (c)
		{
			HmdConsiderMarkerEntityForDelete(c, markingRoot, hitWorld, maxDistM);
			c = c.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool HmdEntityIsUnderMarkingHierarchy(IEntity ent, IEntity markingRoot)
	{
		if (!ent || !markingRoot)
			return false;
		IEntity w = ent;
		while (w)
		{
			if (w == markingRoot)
				return true;
			w = w.GetParent();
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Prefer a child-of-`markingOwner` on the trace bone chain so pool `m_Parent0` matches turret `RplId` (turret HUD scoping).
	protected IEntity HmdResolveReferenceMarkerAttachParent(IEntity markingOwner, IEntity traceHost)
	{
		if (!markingOwner)
			return null;
		if (!traceHost)
			return markingOwner;
		IEntity walk = traceHost;
		while (walk && walk != markingOwner)
		{
			IEntity p = walk.GetParent();
			if (p == markingOwner)
				return walk;
			walk = p;
		}
		return markingOwner;
	}

	//------------------------------------------------------------------------------------------------
	//! Prefer `HMD_LaserMarkingRemoteVehicleComponent` on the local player's turret entity; else first remote marking under vehicle root.
	static HMD_LaserMarkingRemoteVehicleComponent HmdResolveLocalPlayersRemoteMarkingComponent()
	{
		if (!HMD_EntityHmdHelpers.IsLocalMountedInVehicle())
			return null;
		IEntity ch = SCR_PlayerController.GetLocalMainEntity();
		if (!ch)
			ch = SCR_PlayerController.GetLocalControlledEntity();
		if (!ch)
			return null;
		IEntity root = ch.GetRootParent();
		if (!root || root == ch)
			return null;
		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(ch.FindComponent(SCR_CompartmentAccessComponent));
		if (access)
		{
			BaseCompartmentSlot slot = access.GetCompartment();
			if (slot && slot.GetType() == ECompartmentType.TURRET)
			{
				BaseControllerComponent bc = slot.GetController();
				if (bc)
				{
					IEntity seatOwner = bc.GetOwner();
					if (seatOwner)
					{
						HMD_LaserMarkingRemoteVehicleComponent r = HMD_LaserMarkingRemoteVehicleComponent.Cast(seatOwner.FindComponent(HMD_LaserMarkingRemoteVehicleComponent));
						if (r)
							return r;
					}
				}
			}
		}
		return HMD_EntityHmdHelpers.FindFirstLaserMarkingRemoteInHierarchy(root, 24);
	}

	//------------------------------------------------------------------------------------------------
	//! Control hints: resolve remote marking the local player may operate (same rules as gameplay input).
	static HMD_LaserMarkingRemoteVehicleComponent HmdTryResolveLocalEligibleRemoteForControlHints(IEntity vehicleRoot)
	{
		if (!vehicleRoot)
			return null;
		HMD_LaserMarkingRemoteVehicleComponent rm = HmdResolveLocalPlayersRemoteMarkingComponent();
		if (!rm)
			rm = HMD_EntityHmdHelpers.FindFirstLaserMarkingRemoteInHierarchy(vehicleRoot, 24);
		if (!rm || !rm.HmdIsLocalPlayerEligibleForLaserControl())
			return null;
		return rm;
	}

	//------------------------------------------------------------------------------------------------
	//! True when pool `m_Parent0` refers to an entity under this vehicle's first remote marking owner (WP/RP from laser reference).
	static bool HmdLocalVehiclePoolRowParentMatchesRemoteMarkingHierarchy(IEntity vehicleRoot, RplId rowParent0)
	{
		if (!vehicleRoot || rowParent0 == RplId.Invalid())
			return false;
		HMD_LaserMarkingRemoteVehicleComponent rm = HMD_EntityHmdHelpers.FindFirstLaserMarkingRemoteInHierarchy(vehicleRoot, 24);
		if (!rm)
			return false;
		IEntity markingOwner = rm.GetOwner();
		if (!markingOwner)
			return false;
		return HMD_EntityHmdHelpers.EntitySubtreeContainsRplId(markingOwner, rowParent0);
	}
}
