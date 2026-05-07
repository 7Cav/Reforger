//------------------------------------------------------------------------------------------------
//! Informational marker: pushes owner world origin into the global HMD pool on the authority server.
//! **Pool lifecycle:** `OnDelete` calls `HMD_GlobalHmdElementPoolComponent.StaticServerRemoveAllPoolRowsForSource` for `GetOwner()`.
class HMD_MarkerElementBaseComponentClass : ScriptComponentClass
{
}

class HMD_MarkerElementBaseComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.EditBox, "Informational marker class: 0 IFF, 1 Waypoint, 2 Reference, 3 Generic", category: "HMD")]
	protected int m_iInformationalClass;

	[Attribute("MARK", UIWidgets.EditBox, "Pool row label text", category: "HMD")]
	protected string m_sHmdPoolLabel;

	[Attribute("0", UIWidgets.CheckBox, "When set, server defers pool registration until HmdServerFinalizePoolRegistration (spawn-time label/class).", category: "HMD")]
	protected bool m_bHmdDeferPoolRegistration;

	protected int m_iPoolElementId = -1;

	//------------------------------------------------------------------------------------------------
	//! Waypoint / reference (classes 1â€“2): world position is sent once at pool registration; no per-frame server updates.
	protected bool HmdIsStaticInformationalPoolPosition()
	{
		return m_iInformationalClass == 1 || m_iInformationalClass == 2;
	}

	//------------------------------------------------------------------------------------------------
	//! Optional prefab sibling HMD_MarkerGeneratorComponent applies before pool registration.
	void HmdApplyGeneratorOverrides(int infoClass, string poolLabel)
	{
		m_iInformationalClass = infoClass;
		if (!poolLabel.IsEmpty())
			m_sHmdPoolLabel = poolLabel;
	}

	//------------------------------------------------------------------------------------------------
	int HmdGetInformationalClass()
	{
		return m_iInformationalClass;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		HMD_MarkerGeneratorComponent gen = HMD_MarkerGeneratorComponent.Cast(owner.FindComponent(HMD_MarkerGeneratorComponent));
		if (gen)
			gen.PushToMarker(this);
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.FRAME);
		if (Replication.IsServer() && !m_bHmdDeferPoolRegistration)
			HmdRegisterOnce();
	}

	//------------------------------------------------------------------------------------------------
	//! Server: apply class + label then register in the pool (used when `m_bHmdDeferPoolRegistration` is true on spawn).
	void HmdServerFinalizePoolRegistration(int informationalClass, string poolLabel)
	{
		if (!Replication.IsServer())
			return;
		HmdApplyGeneratorOverrides(informationalClass, poolLabel);
		m_bHmdDeferPoolRegistration = false;
		HmdRegisterOnce();
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		if (!Replication.IsServer())
			return;
		if (m_bHmdDeferPoolRegistration)
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
		if (HmdIsStaticInformationalPoolPosition())
		{
			ClearEventMask(owner, EntityEvent.FRAME);
			return;
		}
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
		int hudArgb = 0xFFFFFFFF;
		HMD_MarkerElementLifetimeComponent life = HMD_MarkerElementLifetimeComponent.Cast(this);
		if (life)
			hudArgb = life.HmdGetLifetimePoolHudColorArgb();
		m_iPoolElementId = pool.ServerRegisterElement(owner, EHmdElementKind.INFORMATIONAL, m_iInformationalClass, m_sHmdPoolLabel, 0, p0, p1, false, hudArgb);
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
}
