//------------------------------------------------------------------------------------------------
//! Local IR point light + strobe (NVG-visible). Spawns `RHS_LightEntity` prefab as a child; no replication.
//! **Grenades:** set `m_bUseAttachableBeaconTransmitGate` false — strobe for entity lifetime.
//! **Attachable beacon:** set true — strobe only when `HMD_IffAttachableBeaconComponent` is placed, ON, and has battery.
[BaseContainerProps()]
class HMD_IffIrStrobeVisualComponentClass : ScriptComponentClass
{
}

class HMD_IffIrStrobeVisualComponent : ScriptComponent
{
	[Attribute("{6900000F00000001}Prefabs/Items/Equipment/Nightvision/IFF_IR_Light.et", UIWidgets.ResourceNamePicker, "RHS_LightEntity prefab (IR); empty = no IR.", "et", category: "HMD IR")]
	protected ResourceName m_sIrLightPrefab;

	[Attribute("150", UIWidgets.Auto, "IR strobe ON duration (ms)", category: "HMD IR")]
	protected float m_fIrStrobeOnMs;

	[Attribute("1000", UIWidgets.Auto, "IR strobe OFF duration (ms)", category: "HMD IR")]
	protected float m_fIrStrobeOffMs;

	[Attribute("0 0 0", UIWidgets.EditBox, "World-space offset (m) from owner origin, converted to local spawn.", category: "HMD IR")]
	protected vector m_vIrLightWorldOffset;

	[Attribute("0", UIWidgets.CheckBox, "If true, IR only when attachable beacon on same entity is placed, ON, and has battery.", category: "HMD IR")]
	protected bool m_bUseAttachableBeaconTransmitGate;

	protected IEntity m_pSpawnedIrLight;

	//------------------------------------------------------------------------------------------------
	protected static vector IrWorldOffsetToLocal(vector worldOff, vector parentWorld[4])
	{
		vector localOff;
		localOff[0] = parentWorld[0][0] * worldOff[0] + parentWorld[1][0] * worldOff[1] + parentWorld[2][0] * worldOff[2];
		localOff[1] = parentWorld[0][1] * worldOff[0] + parentWorld[1][1] * worldOff[1] + parentWorld[2][1] * worldOff[2];
		localOff[2] = parentWorld[0][2] * worldOff[0] + parentWorld[1][2] * worldOff[1] + parentWorld[2][2] * worldOff[2];
		return localOff;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ShouldRunIrVisual()
	{
		if (!GetGame() || !GetGame().InPlayMode())
			return false;
		if (Replication.IsServer() && !Replication.IsClient())
			return false;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsIrTransmitting(IEntity owner)
	{
		if (!owner || !owner.GetWorld())
			return false;
		if (!m_bUseAttachableBeaconTransmitGate)
			return true;
		HMD_IffAttachableBeaconComponent b = HMD_IffAttachableBeaconComponent.FindOnEntity(owner);
		if (!b)
			return false;
		if (!b.IsBeaconPlacedInWorld() || !b.IsBeaconActive())
			return false;
		return b.GetBatteryFraction01() > 0.0;
	}

	//------------------------------------------------------------------------------------------------
	protected void StopIrStrobeLoop()
	{
		if (!GetGame())
			return;
		GetGame().GetCallqueue().Remove(IrStrobeOnPhaseEnd);
		GetGame().GetCallqueue().Remove(IrStrobeOffPhaseEnd);
	}

	//------------------------------------------------------------------------------------------------
	protected void IrStrobeOnPhaseEnd()
	{
		IEntity owner = GetOwner();
		if (!owner || !m_pSpawnedIrLight || !IsIrTransmitting(owner))
		{
			StopIrStrobeLoop();
			return;
		}
		RHS_LightEntity le = RHS_LightEntity.Cast(m_pSpawnedIrLight);
		if (le)
			le.SetEnabledWithIRCheck(false);
		float offMs = m_fIrStrobeOffMs;
		if (offMs < 1)
			offMs = 1;
		GetGame().GetCallqueue().CallLater(IrStrobeOffPhaseEnd, offMs, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void IrStrobeOffPhaseEnd()
	{
		IEntity owner = GetOwner();
		if (!owner || !m_pSpawnedIrLight || !IsIrTransmitting(owner))
		{
			StopIrStrobeLoop();
			return;
		}
		RHS_LightEntity le = RHS_LightEntity.Cast(m_pSpawnedIrLight);
		if (le)
			le.SetEnabledWithIRCheck(true);
		float onMs = m_fIrStrobeOnMs;
		if (onMs < 1)
			onMs = 1;
		GetGame().GetCallqueue().CallLater(IrStrobeOnPhaseEnd, onMs, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void StartIrStrobeLoop(IEntity owner)
	{
		StopIrStrobeLoop();
		if (!m_pSpawnedIrLight || !owner)
			return;
		RHS_LightEntity le = RHS_LightEntity.Cast(m_pSpawnedIrLight);
		if (le)
			le.SetEnabledWithIRCheck(true);
		float onMs = m_fIrStrobeOnMs;
		if (onMs < 1)
			onMs = 1;
		GetGame().GetCallqueue().CallLater(IrStrobeOnPhaseEnd, onMs, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void DespawnIrLight()
	{
		StopIrStrobeLoop();
		if (!m_pSpawnedIrLight)
			return;
		IEntity lightEnt = m_pSpawnedIrLight;
		m_pSpawnedIrLight = null;
		SCR_EntityHelper.DeleteEntityAndChildren(lightEnt);
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnIrLightIfNeeded(IEntity owner)
	{
		if (m_pSpawnedIrLight && !m_pSpawnedIrLight.GetWorld())
			m_pSpawnedIrLight = null;
		if (m_pSpawnedIrLight)
			return;
		ResourceName prefabName = m_sIrLightPrefab;
		if (!prefabName || prefabName == string.Empty)
			return;
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		vector worldOff = m_vIrLightWorldOffset;
		vector parentWorld[4];
		owner.GetWorldTransform(parentWorld);
		vector localPos = Vector(0, 0, 0);
		float offSq = worldOff[0] * worldOff[0] + worldOff[1] * worldOff[1] + worldOff[2] * worldOff[2];
		if (offSq > 1e-12)
			localPos = IrWorldOffsetToLocal(worldOff, parentWorld);
		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.LOCAL;
		sp.Parent = owner;
		sp.Transform[0] = Vector(1, 0, 0);
		sp.Transform[1] = Vector(0, 1, 0);
		sp.Transform[2] = Vector(0, 0, 1);
		sp.Transform[3] = localPos;
		IEntity spawned = GetGame().SpawnEntityPrefabEx(prefabName, false, world, sp);
		if (!spawned)
			return;
		m_pSpawnedIrLight = spawned;
		StartIrStrobeLoop(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshIrLightState()
	{
		if (!ShouldRunIrVisual())
		{
			DespawnIrLight();
			return;
		}
		IEntity owner = GetOwner();
		if (!owner)
		{
			DespawnIrLight();
			return;
		}
		if (!IsIrTransmitting(owner))
		{
			DespawnIrLight();
			return;
		}
		if (m_pSpawnedIrLight && !m_pSpawnedIrLight.GetWorld())
			m_pSpawnedIrLight = null;
		if (!m_pSpawnedIrLight)
			SpawnIrLightIfNeeded(owner);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		RefreshIrLightState();
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.FRAME | owner.GetEventMask());
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		DespawnIrLight();
		super.OnDelete(owner);
	}
}
