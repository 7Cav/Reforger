class ParachuteHelperFunctions
{
	static bool IsEntityValid(IEntity entity)
	{
		return entity && entity.GetWorld();
	}

	static float GetHeightAboveTerrain(vector pos)
	{
		float terrainY = SCR_TerrainHelper.GetTerrainY(pos, null, true);
		return pos[1] - terrainY;
	}

	static void DeleteEntityIfValid(IEntity entity)
	{
		if (IsEntityValid(entity))
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}

	static void EjectOccupantFromSlot(BaseCompartmentSlot slot)
	{
		if (!slot)
			return;
		IEntity occupant = slot.GetOccupant();
		if (!occupant)
			return;
		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(occupant.FindComponent(SCR_CompartmentAccessComponent));
		if (access)
			access.AskOwnerToGetOutFromVehicle(EGetOutType.TELEPORT, 0, ECloseDoorAfterActions.LEAVE_OPEN, true, true);
	}

	static ParachuteItemComponent FindParachuteItemInInventory(IEntity pilotEntity, bool unusedOnly = false)
	{
		if (!pilotEntity)
			return null;

		SCR_InventoryStorageManagerComponent invMgr = SCR_InventoryStorageManagerComponent.Cast(
			pilotEntity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!invMgr)
			return null;

		array<IEntity> rootItems = {};
		invMgr.GetItems(rootItems, EStoragePurpose.PURPOSE_ANY);

		foreach (IEntity item : rootItems)
		{
			if (!item)
				continue;

			ParachuteItemComponent pc = ParachuteItemComponent.Cast(item.FindComponent(ParachuteItemComponent));
			if (!pc)
				continue;

			if (unusedOnly && pc.GetParachuteUsed())
				continue;

			return pc;
		}
		return null;
	}

	static BaseCompartmentSlot FindCargoSlotOnEntity(IEntity entity)
	{
		if (!entity)
			return null;
		BaseCompartmentManagerComponent bcm = BaseCompartmentManagerComponent.Cast(entity.FindComponent(BaseCompartmentManagerComponent));
		if (!bcm)
			return null;
		array<BaseCompartmentSlot> slots = {};
		bcm.GetCompartments(slots);
		foreach (BaseCompartmentSlot s : slots)
		{
			if (s && s.GetType() == ECompartmentType.CARGO)
				return s;
		}
		return null;
	}

	static bool IsSlotOccupied(BaseCompartmentSlot slot)
	{
		return slot && slot.IsOccupied();
	}

	static bool IsChuteCompartmentEmpty(IEntity chute)
	{
		return !IsSlotOccupied(FindCargoSlotOnEntity(chute));
	}

	static bool IsWithinTerrainContactThreshold(vector contactPos, float thresholdM)
	{
		if (thresholdM < 0)
			return false;
		return Math.AbsFloat(GetHeightAboveTerrain(contactPos)) <= thresholdM;
	}

	// enable: true = take damage, false = invincible (damage handling disabled)
	static void SetEntityDamageHandling(IEntity entity, bool enable)
	{
		if (!IsEntityValid(entity))
			return;
		DamageManagerComponent dmg = DamageManagerComponent.Cast(entity.FindComponent(DamageManagerComponent));
		if (dmg)
			dmg.EnableDamageHandling(enable);
	}

	static ParachuteComponentExtended GetParachuteComponentFromPilot(IEntity pilot)
	{
		if (!IsEntityValid(pilot))
			return null;
		if (!GetGame())
			return null;
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return null;
		int playerId = pm.GetPlayerIdFromControlledEntity(pilot);
		if (playerId <= 0)
			return null;
		SCR_PlayerController pc = SCR_PlayerController.Cast(pm.GetPlayerController(playerId));
		if (!pc)
			return null;
		return ParachuteComponentExtended.Cast(pc.FindComponent(ParachuteComponent));
	}

	static ParachuteComponent GetParachuteComponentFromSlotOwner(InventoryStorageSlot slot)
	{
		if (!slot)
			return null;

		if (!GetGame())
			return null;

		BaseInventoryStorageComponent storage = slot.GetStorage();
		if (!storage)
			return null;

		IEntity storageOwner = storage.GetOwner();
		if (!storageOwner || !SCR_ChimeraCharacter.Cast(storageOwner))
			return null;

		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return null;

		int playerId = pm.GetPlayerIdFromControlledEntity(storageOwner);
		if (playerId <= 0)
			return null;

		SCR_PlayerController pc = SCR_PlayerController.Cast(pm.GetPlayerController(playerId));
		if (!pc)
			return null;

		return ParachuteComponent.Cast(pc.FindComponent(ParachuteComponent));
	}
}
