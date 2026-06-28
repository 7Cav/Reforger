[ComponentEditorProps(category: "HMD", description: "Enables IR on RHS_LightDevice; SetEnabledWithIRCheck on IR RHS_LightEntity (ancestors and descendants of this entity).")]
class HMD_IRLightBootstrapComponentClass : ScriptComponentClass
{
}

class HMD_IRLightBootstrapComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.CheckBox, "Run ApplyIrEnable() automatically when the entity is ready in play mode", category: "HMD IR")]
	protected bool m_bEnableOnInit;

	[Attribute("1", UIWidgets.CheckBox, "Turn the light device on if it is off", category: "HMD IR")]
	protected bool m_bPowerOnIfOff;

	[Attribute("0", UIWidgets.ComboBox, "Target light type for IR (pick the IR entry your prefab uses)", "", ParamEnumArray.FromEnum(ELightType), category: "HMD IR")]
	protected ELightType m_eIrLightType;

	[Attribute("1", UIWidgets.CheckBox, "After mode switch, set IR visibility from local NV (RHS_IsNVOff). Matches RHS nearby-IR behavior.", category: "HMD IR")]
	protected bool m_bSyncIrVisibilityWithNv;

	[Attribute("1", UIWidgets.CheckBox, "For each IR RHS_LightEntity on parent chain and under this entity, call SetEnabledWithIRCheck(true).", category: "HMD IR")]
	protected bool m_bEnableIrLightsWithIrCheck;

	protected static const int IR_LIGHT_PARENT_WALK_MAX = 32;

	protected RHS_LightDevice m_LightDevice;

	protected void ApplySetEnabledWithIRCheckOnIrLightsUpParents(IEntity start)
	{
		if (!start)
			return;
		IEntity p = start.GetParent();
		int depth = 0;
		while (p && depth < IR_LIGHT_PARENT_WALK_MAX)
		{
			depth++;
			RHS_LightEntity le = RHS_LightEntity.Cast(p);
			if (le && le.IsIR())
				le.SetEnabledWithIRCheck(true);
			p = p.GetParent();
		}
	}

	protected void ApplySetEnabledWithIRCheckOnIrLights(IEntity root)
	{
		if (!root)
			return;
		RHS_LightEntity lightEnt = RHS_LightEntity.Cast(root);
		if (lightEnt && lightEnt.IsIR())
			lightEnt.SetEnabledWithIRCheck(true);
		IEntity child = root.GetChildren();
		while (child)
		{
			ApplySetEnabledWithIRCheckOnIrLights(child);
			child = child.GetSibling();
		}
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!owner)
			return;
		m_LightDevice = RHS_LightDevice.Cast(owner.FindComponent(RHS_LightDevice));
		if (!m_LightDevice)
			return;
		if (!GetGame().InPlayMode())
			return;
		if (m_bEnableOnInit)
			ApplyIrEnable();
	}

	void ApplyIrEnable()
	{
		if (!m_LightDevice)
		{
			IEntity owner = GetOwner();
			if (owner)
				m_LightDevice = RHS_LightDevice.Cast(owner.FindComponent(RHS_LightDevice));
		}
		if (!m_LightDevice)
			return;

		if (m_bPowerOnIfOff && !m_LightDevice.IsTurnedOn())
			m_LightDevice.SwitchDeviceState(true);

		m_LightDevice.SetLightType(m_eIrLightType);

		if (m_bSyncIrVisibilityWithNv)
		{
			SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (pc)
				m_LightDevice.ChangeIRVisibility(!pc.RHS_IsNVOff());
		}

		if (m_bEnableIrLightsWithIrCheck)
		{
			IEntity owner = GetOwner();
			if (owner)
			{
				ApplySetEnabledWithIRCheckOnIrLightsUpParents(owner);
				ApplySetEnabledWithIRCheckOnIrLights(owner);
			}
		}
	}
}
