class IronHorseParachuteFixes_Helper
{
	static bool IsEntityValid(IEntity entity)
	{
		return entity && entity.GetWorld();
	}

	static void DeleteEntityIfValid(IEntity entity)
	{
		if (IsEntityValid(entity))
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}

	// enable: true = take damage, false = invincible (damage handling disabled)
	static void SetEntityDamageHandling(IEntity entity, bool enable)
	{
		if (!IsEntityValid(entity))
			return;

		SCR_CharacterDamageManagerComponent charDmg = SCR_CharacterDamageManagerComponent.Cast(
			entity.FindComponent(SCR_CharacterDamageManagerComponent));
		if (charDmg)
			charDmg.EnableDamageHandling(enable);

		SCR_DamageManagerComponent scrDmg = SCR_DamageManagerComponent.Cast(
			entity.FindComponent(SCR_DamageManagerComponent));
		if (scrDmg)
			scrDmg.EnableDamageHandling(enable);

		DamageManagerComponent dmg = DamageManagerComponent.Cast(entity.FindComponent(DamageManagerComponent));
		if (dmg)
			dmg.EnableDamageHandling(enable);
	}

	static bool IsPilotSeatedInChute(IEntity pilot, IEntity chute)
	{
		if (!IsEntityValid(pilot) || !IsEntityValid(chute))
			return false;

		return SCR_CompartmentAccessComponent.GetVehicleIn(pilot) == chute;
	}

	static void EjectOccupantFromSlot(BaseCompartmentSlot slot)
	{
		if (!slot)
			return;

		IEntity occupant = slot.GetOccupant();
		if (!occupant)
			return;

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(
			occupant.FindComponent(SCR_CompartmentAccessComponent));
		if (access)
			access.AskOwnerToGetOutFromVehicle(EGetOutType.TELEPORT, 0, ECloseDoorAfterActions.LEAVE_OPEN, true, true);
	}

	static BaseCompartmentSlot FindCargoSlotOnEntity(IEntity root)
	{
		if (!root)
			return null;

		return FindCargoCompartmentSlotOnEntityRecursive(root);
	}

	static bool IsChuteCompartmentEmpty(IEntity chute)
	{
		return !IsSlotOccupied(FindCargoSlotOnEntity(chute));
	}

	static bool IsSlotOccupied(BaseCompartmentSlot slot)
	{
		return slot && slot.IsOccupied();
	}

	protected static BaseCompartmentSlot FindCargoCompartmentSlotOnEntityRecursive(IEntity ent)
	{
		if (!ent)
			return null;

		BaseCompartmentSlot slot = FindCargoCompartmentSlotFromManager(ent);
		if (slot)
			return slot;

		IEntity child = ent.GetChildren();
		while (child)
		{
			slot = FindCargoCompartmentSlotOnEntityRecursive(child);
			if (slot)
				return slot;
			child = child.GetSibling();
		}

		return null;
	}

	protected static BaseCompartmentSlot FindCargoCompartmentSlotFromManager(IEntity ent)
	{
		BaseCompartmentManagerComponent bcm = BaseCompartmentManagerComponent.Cast(
			ent.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!bcm)
			bcm = BaseCompartmentManagerComponent.Cast(ent.FindComponent(BaseCompartmentManagerComponent));
		if (!bcm)
			return null;

		array<BaseCompartmentSlot> slots = {};
		bcm.GetCompartments(slots);

		foreach (BaseCompartmentSlot s : slots)
		{
			if (!s)
				continue;

			if (s.GetType() == ECompartmentType.CARGO)
				return s;
		}

		return null;
	}
}
