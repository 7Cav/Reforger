class ADS_AirdropSupplyUserAction : ScriptedUserAction
{
	[Attribute("0 0 0", desc: "Offset from aircraft center [m], local axes: X=right, Y=up, Z=forward")]
	protected vector m_vSpawnOffsetLocal;

	[Attribute("{EF693C583CAB7964}Prefabs/Props/Military/CISS/SupplyDrop/CAV_SuppliesDrop_US_EquipmentBox.et", params: "et")]
	protected ResourceName m_rSupplyPrefab;

	[Attribute("200", desc: "SUPPLIES resource consumed per drop", params: "0 inf")]
	protected int m_iSupplyCost;

	protected IEntity ResolveAircraftEntity()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return null;

		if (owner.FindComponent(BaseCompartmentManagerComponent))
			return owner;

		IEntity parent = owner.GetParent();
		if (parent && parent.FindComponent(BaseCompartmentManagerComponent))
			return parent;

		return owner;
	}

	protected bool ADS_UserIsInAircraftVehicle(IEntity user, IEntity aircraft)
	{
		if (!user || !aircraft)
			return false;

		SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(user);
		if (!ch || !ch.IsInVehicle())
			return false;

		IEntity userVeh = SCR_CompartmentAccessComponent.GetVehicleIn(ch);
		if (!userVeh)
			return false;

		IEntity arRoot = aircraft.GetRootParent();
		if (arRoot) aircraft = arRoot;

		IEntity uvRoot = userVeh.GetRootParent();
		if (uvRoot) userVeh = uvRoot;

		return userVeh == aircraft;
	}

	protected SCR_ResourceComponent ADS_GetVehicleResourceComponent(IEntity aircraft)
	{
		if (!aircraft)
			return null;
		SCR_ResourceComponent res = SCR_ResourceComponent.FindResourceComponent(aircraft, false);
		if (res)
			return res;
		return SCR_ResourceComponent.FindResourceComponent(aircraft, true);
	}

	protected bool ADS_CanAffordDrop(IEntity aircraft, out SCR_ResourceContainer supplyContainer)
	{
		supplyContainer = null;
		SCR_ResourceComponent res = ADS_GetVehicleResourceComponent(aircraft);
		if (!res)
			return false;

		SCR_ResourceContainer ctr = res.GetContainer(EResourceType.SUPPLIES);
		if (!ctr)
			return false;

		supplyContainer = ctr;
		return ctr.GetResourceValue() >= m_iSupplyCost;
	}

	override bool CanBeShownScript(IEntity user)
	{
		IEntity aircraft = ResolveAircraftEntity();
		return ADS_UserIsInAircraftVehicle(user, aircraft);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		IEntity aircraft = ResolveAircraftEntity();
		if (!ADS_UserIsInAircraftVehicle(user, aircraft))
			return false;

		SCR_ResourceContainer ctr;
		if (!ADS_CanAffordDrop(aircraft, ctr))
		{
			SetCannotPerformReason(string.Format("Insufficient supplies (%1 required)", m_iSupplyCost));
			return false;
		}

		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		IEntity aircraft = ResolveAircraftEntity();
		if (!aircraft)
			return;

		SCR_ResourceContainer supplyCtr;
		if (!ADS_CanAffordDrop(aircraft, supplyCtr))
			return;

		if (!supplyCtr.DecreaseResourceValue(m_iSupplyCost, true))
			return;

		Resource res = Resource.Load(m_rSupplyPrefab);
		if (!res || !res.IsValid())
			return;

		vector mat[4];
		aircraft.GetWorldTransform(mat);

		vector off = m_vSpawnOffsetLocal;
		mat[3] = mat[3] + mat[0] * off[0] + mat[1] * off[1] + mat[2] * off[2];

		EntitySpawnParams esp = new EntitySpawnParams();
		esp.TransformMode = ETransformMode.WORLD;
		esp.Transform = mat;

		ChimeraWorld world = GetGame().GetWorld();
		if (!world)
			return;

		GetGame().SpawnEntityPrefab(res, world, esp);
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Drop Infantry Resupply";
		return true;
	}

	override bool CanBroadcastScript()
	{
		return true;
	}
}
