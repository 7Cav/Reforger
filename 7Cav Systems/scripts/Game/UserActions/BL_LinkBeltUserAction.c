class BL_LinkBeltUserAction: ScriptedUserAction
{
	protected const float DEFAULT_LINK_RANGE_METERS = 2.0;
	protected const string WRONG_AMMO_REASON = "Link Belts: Wrong Ammo Type";

	protected IEntity ResolveWeaponEntity(IEntity owner)
	{
		if (!owner)
			return null;
		IEntity weapon = owner;
		IEntity parent = owner.GetParent();
		if (parent && parent.FindComponent(BL_BeltLinkWeaponComponent))
			weapon = parent;
		return weapon;
	}

	override bool CanBroadcastScript()
	{
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.PerformAction(pOwnerEntity, pUserEntity);
		if (!Replication.IsServer())
			return;

		IEntity weaponEntity = ResolveWeaponEntity(pOwnerEntity);
		if (!weaponEntity)
			return;

		DoLinkBelt(weaponEntity, pUserEntity);
	}

	protected void DoLinkBelt(IEntity weaponEntity, IEntity linkerEntity)
	{
		BaseWeaponComponent weaponComp = BaseWeaponComponent.Cast(weaponEntity.FindComponent(BaseWeaponComponent));
		if (!weaponComp)
			return;

		array<BaseMuzzleComponent> muzzles = new array<BaseMuzzleComponent>();
		weaponComp.GetMuzzlesList(muzzles);
		BaseMuzzleComponent primaryMuzzle = null;
		for (int i = 0; i < muzzles.Count(); i++)
		{
			BaseMuzzleComponent muzzle = muzzles.Get(i);
			if (muzzle && muzzle.GetMuzzleType() == EMuzzleType.MT_BaseMuzzle)
			{
				primaryMuzzle = muzzle;
				break;
			}
		}
		if (!primaryMuzzle)
			return;

		BaseMagazineComponent weaponMag = primaryMuzzle.GetMagazine();
		if (!weaponMag)
			return;

		SCR_CharacterControllerComponent linkerController = SCR_CharacterControllerComponent.Cast(linkerEntity.FindComponent(SCR_CharacterControllerComponent));
		if (!linkerController)
			return;

		InventoryStorageManagerComponent invManager = linkerController.GetInventoryStorageManager();
		if (!invManager)
			return;

		BaseMagazineWell magWell = primaryMuzzle.GetMagazineWell();
		ResourceName requiredPrefab = ResourceName.Empty;
		IEntity weaponMagEntity = weaponMag.GetOwner();
		if (weaponMagEntity)
			requiredPrefab = GetMagazinePrefabFromEntity(weaponMagEntity);

		BaseInventoryStorageComponent foundStorage = null;
		IEntity sourceMagEntity = FindMagazineForLinking(invManager, magWell, requiredPrefab, foundStorage);
		if (!sourceMagEntity)
			return;

		BaseMagazineComponent sourceMagComp = BaseMagazineComponent.Cast(sourceMagEntity.FindComponent(BaseMagazineComponent));
		if (!sourceMagComp)
			return;

		int weaponAmmo = weaponMag.GetAmmoCount();
		int weaponMax = weaponMag.GetMaxAmmoCount();
		int sourceAmmo = sourceMagComp.GetAmmoCount();
		int space = weaponMax - weaponAmmo;
		if (space > 0 && sourceAmmo > 0)
		{
			int toTransfer = space;
			if (toTransfer > sourceAmmo)
				toTransfer = sourceAmmo;
			weaponMag.SetAmmoCount(weaponAmmo + toTransfer);
			int remaining = sourceAmmo - toTransfer;
			sourceMagComp.SetAmmoCount(remaining);
			if (remaining <= 0 && foundStorage && invManager.CanRemoveItemFromStorage(sourceMagEntity, foundStorage))
			{
				invManager.TryRemoveItemFromStorage(sourceMagEntity, foundStorage);
				RplComponent.DeleteRplEntity(sourceMagEntity, false);
			}
			return;
		}

		IEntity muzzleEntity = primaryMuzzle.GetOwner();
		AttachmentSlotComponent magSlot = null;
		if (muzzleEntity)
			magSlot = FindMagazineSlot(muzzleEntity, magWell);
		if (!magSlot)
			magSlot = FindMagazineSlot(weaponEntity, magWell);
		if (!magSlot)
			return;

		IEntity currentMagEntity = magSlot.GetAttachedEntity();
		IEntity partialMagEntity = null;
		if (currentMagEntity)
		{
			magSlot.SetAttachment(null);
			partialMagEntity = currentMagEntity;
		}
		if (foundStorage && invManager.CanRemoveItemFromStorage(sourceMagEntity, foundStorage))
			invManager.TryRemoveItemFromStorage(sourceMagEntity, foundStorage);
		magSlot.SetAttachment(sourceMagEntity);
		if (partialMagEntity)
		{
			if (!invManager.TryInsertItem(partialMagEntity))
				RplComponent.DeleteRplEntity(partialMagEntity, false);
		}
	}

	protected ResourceName GetMagazinePrefabFromEntity(IEntity magEntity)
	{
		if (!magEntity)
			return ResourceName.Empty;
		EntityPrefabData pfd = magEntity.GetPrefabData();
		if (!pfd)
			return ResourceName.Empty;
		return pfd.GetPrefabName();
	}

	protected bool IsMagazineWellCompatible(BaseMagazineWell itemWell, BaseMagazineWell weaponWell)
	{
		if (!itemWell || !weaponWell)
			return false;
		return itemWell.Type() == weaponWell.Type() || itemWell.Type().IsInherited(weaponWell.Type()) || weaponWell.Type().IsInherited(itemWell.Type());
	}

	protected bool HasAnyCompatibleMagazineWithAmmo(InventoryStorageManagerComponent invManager, BaseMagazineWell magazineWell)
	{
		if (!invManager || !magazineWell)
			return false;

		array<BaseInventoryStorageComponent> storages = new array<BaseInventoryStorageComponent>();
		invManager.GetStorages(storages);

		for (int s = 0; s < storages.Count(); s++)
		{
			BaseInventoryStorageComponent storage = storages.Get(s);
			if (!storage)
				continue;

			for (int i = 0; i < storage.GetSlotsCount(); i++)
			{
				IEntity itemEntity = storage.Get(i);
				if (!itemEntity)
					continue;

				BaseMagazineComponent magComp = BaseMagazineComponent.Cast(itemEntity.FindComponent(BaseMagazineComponent));
				if (!magComp)
					continue;

				BaseMagazineWell magWell = magComp.GetMagazineWell();
				if (!magWell)
					continue;

				if (!IsMagazineWellCompatible(magWell, magazineWell))
					continue;

				if (magComp.GetAmmoCount() > 0)
					return true;
			}
		}
		return false;
	}

	protected IEntity FindMagazineForLinking(InventoryStorageManagerComponent invManager, BaseMagazineWell magazineWell, ResourceName requiredPrefab, out BaseInventoryStorageComponent outStorage)
	{
		outStorage = null;
		if (!invManager || !magazineWell)
			return null;

		bool requireMatch = !requiredPrefab.IsEmpty();

		array<BaseInventoryStorageComponent> storages = new array<BaseInventoryStorageComponent>();
		invManager.GetStorages(storages);

		IEntity bestEntity = null;
		BaseInventoryStorageComponent bestStorage = null;
		int bestAmmo = -1;

		for (int s = 0; s < storages.Count(); s++)
		{
			BaseInventoryStorageComponent storage = storages.Get(s);
			if (!storage)
				continue;

			for (int i = 0; i < storage.GetSlotsCount(); i++)
			{
				IEntity itemEntity = storage.Get(i);
				if (!itemEntity)
					continue;

				BaseMagazineComponent magComp = BaseMagazineComponent.Cast(itemEntity.FindComponent(BaseMagazineComponent));
				if (!magComp)
					continue;

				BaseMagazineWell magWell = magComp.GetMagazineWell();
				if (!magWell)
					continue;

				if (!IsMagazineWellCompatible(magWell, magazineWell))
					continue;

				int ammo = magComp.GetAmmoCount();
				if (ammo <= 0)
					continue;

				if (requireMatch)
				{
					ResourceName itemPrefab = GetMagazinePrefabFromEntity(itemEntity);
					if (itemPrefab != requiredPrefab)
						continue;
				}

				if (ammo > bestAmmo)
				{
					bestAmmo = ammo;
					bestEntity = itemEntity;
					bestStorage = storage;
				}
			}
		}

		outStorage = bestStorage;
		return bestEntity;
	}

	protected AttachmentSlotComponent FindMagazineSlot(IEntity weaponEntity, BaseMagazineWell magazineWell)
	{
		if (!weaponEntity || !magazineWell)
			return null;
		return FindMagazineSlotRecursive(weaponEntity, magazineWell);
	}

	protected AttachmentSlotComponent FindMagazineSlotRecursive(IEntity entity, BaseMagazineWell magazineWell)
	{
		if (!entity || !magazineWell)
			return null;

		array<Managed> slots = new array<Managed>();
		entity.FindComponents(AttachmentSlotComponent, slots);
		for (int i = 0; i < slots.Count(); i++)
		{
			AttachmentSlotComponent slot = AttachmentSlotComponent.Cast(slots.Get(i));
			if (!slot)
				continue;
			BaseAttachmentType slotType = slot.GetAttachmentSlotType();
			if (!slotType)
				continue;
			if (slotType.Type().IsInherited(magazineWell.Type()) || magazineWell.Type().IsInherited(slotType.Type()))
				return slot;
		}

		IEntity child = entity.GetChildren();
		while (child)
		{
			AttachmentSlotComponent found = FindMagazineSlotRecursive(child, magazineWell);
			if (found)
				return found;
			child = child.GetSibling();
		}
		return null;
	}

	protected float GetLinkRange(IEntity weaponEntity)
	{
		BL_BeltLinkWeaponComponent comp = BL_BeltLinkWeaponComponent.Cast(weaponEntity.FindComponent(BL_BeltLinkWeaponComponent));
		if (comp)
			return comp.GetLinkRange();
		return DEFAULT_LINK_RANGE_METERS;
	}

	protected bool EvaluateLinkBeltContext(IEntity user, out BaseMuzzleComponent outMuzzle, out InventoryStorageManagerComponent outInv, out BaseMagazineWell outMagWell, out ResourceName outRequiredPrefab)
	{
		outMuzzle = null;
		outInv = null;
		outMagWell = null;
		outRequiredPrefab = ResourceName.Empty;

		if (!user)
			return false;

		IEntity ownerEnt = GetOwner();
		IEntity weaponEntity = ResolveWeaponEntity(ownerEnt);
		if (!weaponEntity)
			return false;

		if (!weaponEntity.FindComponent(BL_BeltLinkWeaponComponent))
			return false;

		IEntity weaponRoot = weaponEntity.GetRootParent();
		if (weaponRoot == user)
			return false;

		float distToHelper = vector.Distance(user.GetOrigin(), ownerEnt.GetOrigin());
		float linkRange = GetLinkRange(weaponEntity);
		if (distToHelper > linkRange)
			return false;

		BaseWeaponComponent weaponComp = BaseWeaponComponent.Cast(weaponEntity.FindComponent(BaseWeaponComponent));
		if (!weaponComp)
			return false;

		array<BaseMuzzleComponent> muzzles = new array<BaseMuzzleComponent>();
		weaponComp.GetMuzzlesList(muzzles);
		for (int i = 0; i < muzzles.Count(); i++)
		{
			BaseMuzzleComponent muzzle = muzzles.Get(i);
			if (muzzle && muzzle.GetMuzzleType() == EMuzzleType.MT_BaseMuzzle)
			{
				outMuzzle = muzzle;
				break;
			}
		}
		if (!outMuzzle)
			return false;

		outMagWell = outMuzzle.GetMagazineWell();
		if (!outMagWell)
			return false;

		BaseMagazineComponent weaponMag = outMuzzle.GetMagazine();
		if (!weaponMag)
			return false;

		IEntity weaponMagEntity = weaponMag.GetOwner();
		if (weaponMagEntity)
			outRequiredPrefab = GetMagazinePrefabFromEntity(weaponMagEntity);

		SCR_CharacterControllerComponent linkerController = SCR_CharacterControllerComponent.Cast(user.FindComponent(SCR_CharacterControllerComponent));
		if (!linkerController)
			return false;

		outInv = linkerController.GetInventoryStorageManager();
		if (!outInv)
			return false;

		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		BaseMuzzleComponent primaryMuzzle = null;
		InventoryStorageManagerComponent invManager = null;
		BaseMagazineWell magWell = null;
		ResourceName requiredPrefab = ResourceName.Empty;

		if (!EvaluateLinkBeltContext(user, primaryMuzzle, invManager, magWell, requiredPrefab))
			return false;

		return HasAnyCompatibleMagazineWithAmmo(invManager, magWell);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		BaseMuzzleComponent primaryMuzzle = null;
		InventoryStorageManagerComponent invManager = null;
		BaseMagazineWell magWell = null;
		ResourceName requiredPrefab = ResourceName.Empty;

		if (!EvaluateLinkBeltContext(user, primaryMuzzle, invManager, magWell, requiredPrefab))
			return false;

		if (!HasAnyCompatibleMagazineWithAmmo(invManager, magWell))
			return false;

		BaseInventoryStorageComponent dummyStorage;
		IEntity magForLink = FindMagazineForLinking(invManager, magWell, requiredPrefab, dummyStorage);
		if (magForLink)
			return true;

		if (!requiredPrefab.IsEmpty())
		{
			SetCannotPerformReason(WRONG_AMMO_REASON);
			return false;
		}

		return false;
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Link belt";
		return true;
	}
}
