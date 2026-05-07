//------------------------------------------------------------------------------------------------
//! Gate: local player is in the same vehicle root as this component owner and compartment name matches a parsed token.
class HMD_ElementVisualizationVehicleComponentClass : HMD_ElementVisualizationBaseComponentClass
{
}

class HMD_ElementVisualizationVehicleComponent : HMD_ElementVisualizationBaseComponent
{
	[Attribute("pilot gunner", UIWidgets.EditBox, "Space-separated compartment unique names", category: "HMD")]
	protected string m_sEligibleSeatNames;

	[Attribute("0", UIWidgets.CheckBox, "When set, this hull requires HMD helmet policy for gated seats (global Configs/HMD/VehicleHelmetPrefabs.conf: prefab list + enforce; HMD_HelmetCapabilityComponent on attachments).", category: "HMD")]
	protected bool m_bEnforceHmdHelmetInVehicles;

	protected ref array<string> m_aSeatTokens = {};

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		HmdTokenizeWhitespaceSeatNames(m_sEligibleSeatNames, m_aSeatTokens);
	}

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
	//! True when `controlled` is crewed on the same root as this component owner and passes seat + helmet policy.
	bool HmdEvaluateLocalVehicleHudEligibility(IEntity controlled)
	{
		if (!controlled)
			return false;
		IEntity hull = GetOwner();
		if (!hull)
			return false;
		if (controlled.GetRootParent() != hull.GetRootParent())
			return false;
		if (m_aSeatTokens.IsEmpty())
			return true;
		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(controlled.FindComponent(SCR_CompartmentAccessComponent));
		if (!access)
			return false;
		BaseCompartmentSlot slot = access.GetCompartment();
		if (!slot)
			return false;
		string slotName = HmdGetSlotIdentityName(slot);
		if (slotName.IsEmpty())
			return false;
		foreach (string tok : m_aSeatTokens)
		{
			if (tok == slotName)
				return HmdPassesHelmetStub(controlled);
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Prefer compartment Unique name; if empty, display name (matches LaserFixesAgain slot identity for hints).
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
	//! Tooltip / HUD-toggle eligibility: resolve vehicle from compartment (LaserFixesAgain pattern), then DFS for this component type on the vehicle root.
	static bool StaticIsLocalEligibleForVehicleHmdHud(IEntity controlled)
	{
		if (!controlled)
			return false;
		SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(controlled);
		if (!ch || !ch.IsInVehicle())
			return false;
		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(controlled.FindComponent(SCR_CompartmentAccessComponent));
		if (!access)
			return false;
		BaseCompartmentSlot slot = access.GetCompartment();
		if (!slot || !slot.GetOwner())
			return false;
		IEntity vehicleRoot = slot.GetOwner().GetRootParent();
		if (!vehicleRoot)
			vehicleRoot = slot.GetOwner();
		HMD_ElementVisualizationVehicleComponent pol = HMD_ElementVisualizationVehicleComponent.Cast(HMD_EntityHmdHelpers.FindComponentInHierarchy(vehicleRoot, HMD_ElementVisualizationVehicleComponent));
		if (!pol)
			return false;
		return pol.HmdEvaluateLocalVehicleHudEligibility(controlled);
	}

	//------------------------------------------------------------------------------------------------
	override bool HmdIsVisualizationGateActive()
	{
		//! Match hint / `HMD_EntityHmdHelpers`: main entity first (e.g. crewed seat), else controlled.
		IEntity controlled = SCR_PlayerController.GetLocalMainEntity();
		if (!controlled)
			controlled = SCR_PlayerController.GetLocalControlledEntity();
		return HmdEvaluateLocalVehicleHudEligibility(controlled);
	}

	//------------------------------------------------------------------------------------------------
	protected bool HmdPassesHelmetStub(IEntity controlled)
	{
		if (!m_bEnforceHmdHelmetInVehicles)
			return true;
		SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(controlled);
		if (!ch)
			return false;
		return HMD_HmdHelmetPolicy.CharacterHasHmdHelmetCapability(ch);
	}
}
