//------------------------------------------------------------------------------------------------
//! Optional server-side resync: traces from a bone (or entity forward) into the world and feeds hits into HMD_DesignationElementBaseComponent.
//! Leave m_sTraceBoneName empty to disable. Animation host: first entity under owner (or up parents) whose skeleton defines the bone (`ResolveBoneAnimationHost`).
class HMD_DesignatorRayResyncComponentClass : ScriptComponentClass
{
}

class HMD_DesignatorRayResyncComponent : ScriptComponent
{
	[Attribute("", UIWidgets.EditBox, "Bone name for origin and forward; empty disables this component", category: "HMD")]
	protected string m_sTraceBoneName;

	[Attribute("2000", UIWidgets.EditBox, "Max ray length in meters", category: "HMD")]
	protected float m_fMaxRangeM;

	[Attribute("0", UIWidgets.CheckBox, "If set, owner must be a character and the held designation is updated instead of a component on the owner entity", category: "HMD")]
	protected bool m_bHeldGadgetOnCharacter;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (m_sTraceBoneName.IsEmpty())
			return;
		SetEventMask(owner, EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		if (!Replication.IsServer())
			return;
		if (!owner || m_sTraceBoneName.IsEmpty())
			return;
		HMD_DesignationElementBaseComponent des;
		if (m_bHeldGadgetOnCharacter)
		{
			SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(owner);
			if (!ch)
				return;
			des = HMD_EntityHmdHelpers.FindHeldDesignationOnCharacter(ch);
		}
		else
		{
			des = HMD_DesignationElementBaseComponent.Cast(owner.FindComponent(HMD_DesignationElementBaseComponent));
		}
		if (!des || !des.IsDesignating())
			return;
		IEntity animHost = HMD_DesignatorRayTraceUtils.ResolveBoneAnimationHost(owner, m_sTraceBoneName);
		if (!animHost)
			animHost = owner;
		vector origin = HMD_DesignatorRayTraceUtils.GetBoneWorldPosition(animHost, m_sTraceBoneName);
		vector dir = HMD_DesignatorRayTraceUtils.GetBoneWorldForward(animHost, m_sTraceBoneName);
		vector hit;
		float frac;
		if (!HMD_DesignatorRayTraceUtils.TraceRay(animHost, origin, dir, m_fMaxRangeM, hit, frac))
			return;
		des.HmdServerRefreshDesignatedPosition(hit);
	}
}
