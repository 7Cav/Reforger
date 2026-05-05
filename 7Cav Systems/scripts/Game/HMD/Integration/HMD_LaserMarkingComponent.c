//------------------------------------------------------------------------------------------------
//! Vehicle / turret laser marking: seat allow-list, optional helmet policy, and **turret / FLIR bone** trace.
//! Prefer **`HMD_LaserMarkingRemoteVehicleComponent`** or **`HMD_LaserMarkingOccupiedTurretComponent`** on prefabs instead of this class when possible.
//! Extends **`HMD_LaserMarkingCoreComponent`** for shared designation spawn + laser code replication.
[BaseContainerProps()]
class HMD_LaserMarkingComponentClass : HMD_LaserMarkingCoreComponentClass
{
}

class HMD_LaserMarkingComponent : HMD_LaserMarkingCoreComponent
{
	[Attribute("", UIWidgets.EditBox, "Space-separated compartment unique names allowed to toggle marking and change laser code (empty = any seat on this vehicle root). Same identity rules as `HMD_ElementVisualizationVehicleComponent` (Unique name, else display name).", category: "HMD Laser control")]
	protected string m_sLaserControlSeatNames;

	[Attribute("0", UIWidgets.CheckBox, "When set, laser control seats require HMD helmet capability (same policy shape as vehicle HUD visualization).", category: "HMD Laser control")]
	protected bool m_bEnforceHelmetForLaserControlSeats;

	[Attribute("", UIWidgets.EditBox, "Turret / FLIR bone for designation ray origin + forward (empty = no trace / no spawn)", category: "HMD Designation trace")]
	protected string m_sDesignationTraceBoneName;

	protected ref array<string> m_aLaserControlSeatTokens = {};

	//------------------------------------------------------------------------------------------------
	protected static void HmdTokenizeWhitespaceSeatNames(string src, notnull array<string> outTok)
	{
		outTok.Clear();
		if (!src || src.IsEmpty())
			return;
		string work = src;
		work.TrimInPlace();
		const string delim = " ";
		while (!work.IsEmpty())
		{
			int idx = work.IndexOf(delim);
			if (idx < 0)
			{
				outTok.Insert(work);
				return;
			}
			string head = work.Substring(0, idx);
			head.TrimInPlace();
			if (!head.IsEmpty())
				outTok.Insert(head);
			int restStart = idx + 1;
			int restLen = work.Length() - restStart;
			if (restLen <= 0)
			{
				work = "";
				continue;
			}
			work = work.Substring(restStart, restLen);
			work.TrimInPlace();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static string HmdGetSlotIdentityName(BaseCompartmentSlot slot)
	{
		if (!slot)
			return "";
		string un = slot.GetCompartmentUniqueName();
		if (un && !un.IsEmpty())
			return un;
		string dn = slot.GetCompartmentName(true);
		if (dn && !dn.IsEmpty())
			return dn;
		return "";
	}

	//------------------------------------------------------------------------------------------------
	protected bool HmdPassesHelmetForLaserControl(IEntity controlled)
	{
		if (!m_bEnforceHelmetForLaserControlSeats)
			return true;
		SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(controlled);
		if (!ch)
			return false;
		return HMD_HmdHelmetPolicy.CharacterHasHmdHelmetCapability(ch);
	}

	//------------------------------------------------------------------------------------------------
	protected bool HmdControlledSharesMarkingVehicleRoot(IEntity controlled)
	{
		if (!controlled)
			return false;
		IEntity markingEnt = GetOwner();
		if (!markingEnt)
			return false;
		return controlled.GetRootParent() == markingEnt.GetRootParent();
	}

	//------------------------------------------------------------------------------------------------
	protected bool HmdEvaluateLaserControlByConfiguredSeatTokens(IEntity controlled)
	{
		if (!HmdControlledSharesMarkingVehicleRoot(controlled))
			return false;
		if (m_aLaserControlSeatTokens.IsEmpty())
			return HmdPassesHelmetForLaserControl(controlled);
		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(controlled.FindComponent(SCR_CompartmentAccessComponent));
		if (!access)
			return false;
		BaseCompartmentSlot slot = access.GetCompartment();
		if (!slot)
			return false;
		string slotName = HmdGetSlotIdentityName(slot);
		if (slotName.IsEmpty())
			return false;
		foreach (string tok : m_aLaserControlSeatTokens)
		{
			if (tok == slotName)
				return HmdPassesHelmetForLaserControl(controlled);
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool HmdEvaluateLocalLaserControlEligibility(IEntity controlled)
	{
		return HmdEvaluateLaserControlByConfiguredSeatTokens(controlled);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		HmdTokenizeWhitespaceSeatNames(m_sLaserControlSeatNames, m_aLaserControlSeatTokens);
		super.OnPostInit(owner);
	}

	//------------------------------------------------------------------------------------------------
	override protected bool HmdTryResolveDesignationTrace(IEntity markingOwner, out vector origin, out vector dir, out IEntity traceHost)
	{
		origin = vector.Zero;
		dir = vector.Zero;
		traceHost = null;
		if (!markingOwner || m_sDesignationTraceBoneName.IsEmpty())
			return false;
		IEntity animHost = HMD_DesignatorRayTraceUtils.ResolveBoneAnimationHost(markingOwner, m_sDesignationTraceBoneName);
		if (!animHost)
			animHost = markingOwner;
		traceHost = animHost;
		origin = HMD_DesignatorRayTraceUtils.GetBoneWorldPosition(animHost, m_sDesignationTraceBoneName);
		dir = HMD_DesignatorRayTraceUtils.GetBoneWorldForward(animHost, m_sDesignationTraceBoneName);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool HmdTryGetClientHudDesignationRay(out vector origin, out vector dir, out IEntity traceHost)
	{
		return HmdTryResolveDesignationTrace(GetOwner(), origin, dir, traceHost);
	}
}
