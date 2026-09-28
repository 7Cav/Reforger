//------------------------------------------------------------------------------------------------
//! One weighted entry in a uniform pool.
[BaseContainerProps(), BaseContainerCustomTitleField("m_sPrefab")]
class CPO_UniformVariant
{
	[Attribute(desc: "Clothing prefab. Must be valid for the slot whose list it sits in - a jacket in the trouser pool simply fails to equip.", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et")]
	ResourceName m_sPrefab;

	[Attribute(defvalue: "1", params: "0 inf", desc: "Relative chance against the other entries in the same list. 0 parks an entry without deleting it.")]
	int m_iWeight;
}

[ComponentEditorProps(category: "GameScripted/7Cav PubFac", description: "Rolls the character's jacket and trousers from weighted pools when they spawn")]
class CPO_RandomUniformComponentClass : ScriptComponentClass
{
}

//------------------------------------------------------------------------------------------------
//! Per-soldier uniform variety: rolls a jacket and a pair of trousers out of weighted pools when
//! the character spawns, so a squad is not eight men in the identical shirt.
//!
//! Why this is a component and not prefab data. Vanilla ships exactly one randomisation hook for
//! characters, SCR_EditableEntityVariantData, and it swaps the WHOLE prefab: Game Master's
//! "Randomized US Soldier" entry rolls between Rifleman, Grenadier, Medic and so on. It is read
//! by the placing editor and by SCR_EntityCatalogSpawnerData, and by nothing else. SCR_AIGroup
//! spawns m_aUnitPrefabSlots verbatim, so a group never touches the variant system at all - which
//! is precisely the case that matters here, because the AI support groups are the reason this
//! faction exists. Nothing in the base game varies clothing within one role, so this is ours.
//!
//! The pools live on Character_CPO_Base (Army line troops) and are overridden wholesale on
//! Character_CPO_SF_Base (rolled and pushed sleeves only, knee pads, untucked tee). Every kit
//! inherits from one of those two, so all of them get the treatment with no per-role work, and
//! a single prefab can opt out by clearing m_bEnabled.
//!
//! Slot-to-area mapping, read off vanilla's Character_Base: Hat is LoadoutHeadCoverArea, Jacket
//! is LoadoutJacketArea, Pants is LoadoutPantsArea. Worth checking there before adding a pool for
//! another slot, since the names differ from the areas (ArmoredVest is LoadoutArmoredVestSlotArea,
//! not LoadoutVestArea, and Vest is a separate slot again).
//!
//! Server only. The swap is an ordinary inventory operation, so the chosen garment replicates by
//! itself; a proxy running its own roll would disagree with the server about what the man is
//! wearing.
class CPO_RandomUniformComponent : ScriptComponent
{
	[Attribute(defvalue: "1", desc: "Off leaves the loadout's own jacket and trousers exactly as authored.", category: "7Cav PubFac")]
	protected bool m_bEnabled;

	[Attribute(desc: "Jacket pool. Empty leaves the jacket the loadout set.", category: "7Cav PubFac")]
	protected ref array<ref CPO_UniformVariant> m_aJackets;

	[Attribute(desc: "Trouser pool. Empty leaves the trousers the loadout set.", category: "7Cav PubFac")]
	protected ref array<ref CPO_UniformVariant> m_aTrousers;

	[Attribute(desc: "Headgear pool. Empty leaves the helmet the loadout set.", category: "7Cav PubFac")]
	protected ref array<ref CPO_UniformVariant> m_aHelmets;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!m_bEnabled || SCR_Global.IsEditMode(owner))
			return;

		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (rpl && rpl.IsProxy())
			return;

		// One frame late, deliberately. BaseLoadoutManagerComponent dresses the character during
		// entity init and the engine fills InitialInventoryItems around the same point; rolling
		// after both have run means the pocket contents we have to carry across already exist.
		GetGame().GetCallqueue().CallLater(CPO_ApplyUniform, 0, false);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(CPO_ApplyUniform);
		GetGame().GetCallqueue().Remove(CPO_VerifyDressed);

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void CPO_ApplyUniform()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		SCR_CharacterInventoryStorageComponent storage = SCR_CharacterInventoryStorageComponent.Cast(owner.FindComponent(SCR_CharacterInventoryStorageComponent));
		if (!manager || !storage)
			return;

		CPO_SwapArea(owner, manager, storage, LoadoutJacketArea, m_aJackets);
		CPO_SwapArea(owner, manager, storage, LoadoutPantsArea, m_aTrousers);
		CPO_SwapArea(owner, manager, storage, LoadoutHeadCoverArea, m_aHelmets);

		// Guard for spawn paths that skip or lose the loadout: a few seconds after spawn any
		// area that is still bare is dressed from its pool. The swap above already handles an
		// empty slot, so this re-uses it once and logs the victim, making a silent path name itself.
		GetGame().GetCallqueue().CallLater(CPO_VerifyDressed, 3000, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Second pass a few seconds after spawn: dresses any area still bare and logs it. The loadout
	//! applies during entity init, so an empty jacket or pants slot here means some spawn path
	//! skipped it or something stripped it. Dressing from the pool makes the man whole, and the log
	//! line names the prefab and position so the offending path can be found in the server log.
	protected void CPO_VerifyDressed()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		SCR_CharacterInventoryStorageComponent storage = SCR_CharacterInventoryStorageComponent.Cast(owner.FindComponent(SCR_CharacterInventoryStorageComponent));
		if (!manager || !storage)
			return;

		CPO_VerifyArea(owner, manager, storage, LoadoutJacketArea, m_aJackets, "jacket");
		CPO_VerifyArea(owner, manager, storage, LoadoutPantsArea, m_aTrousers, "trousers");
		CPO_VerifyArea(owner, manager, storage, LoadoutHeadCoverArea, m_aHelmets, "headgear");
	}

	//------------------------------------------------------------------------------------------------
	protected void CPO_VerifyArea(notnull IEntity owner, notnull SCR_InventoryStorageManagerComponent manager, notnull SCR_CharacterInventoryStorageComponent storage, typename areaType, array<ref CPO_UniformVariant> pool, string areaName)
	{
		if (!pool || pool.IsEmpty())
			return;

		LoadoutSlotInfo slot = storage.GetSlotFromArea(areaType);
		if (!slot || slot.GetAttachedEntity())
			return;

		vector pos = owner.GetOrigin();
		Print("[7Cav PubFac] bare " + areaName + " after spawn on " + SCR_ResourceNameUtils.GetPrefabName(owner) + " at " + ((int)pos[0]).ToString() + "," + ((int)pos[2]).ToString() + " - dressing from the pool", LogLevel.WARNING);
		CPO_SwapArea(owner, manager, storage, areaType, pool);
	}

	//------------------------------------------------------------------------------------------------
	//! Replaces whatever occupies one loadout area with a fresh roll from the pool.
	protected void CPO_SwapArea(notnull IEntity owner, notnull SCR_InventoryStorageManagerComponent manager, notnull SCR_CharacterInventoryStorageComponent storage, typename areaType, array<ref CPO_UniformVariant> pool)
	{
		if (!pool || pool.IsEmpty())
			return;

		LoadoutSlotInfo slot = storage.GetSlotFromArea(areaType);
		if (!slot)
			return;

		IEntity worn = slot.GetAttachedEntity();

		ResourceName picked = CPO_Pick(pool);
		if (picked.IsEmpty() || picked == SCR_ResourceNameUtils.GetPrefabName(worn))
			return;

		Resource resource = Resource.Load(picked);
		if (!resource.IsValid())
			return;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		owner.GetWorldTransform(spawnParams.Transform);

		IEntity replacement = GetGame().SpawnEntityPrefab(resource, owner.GetWorld(), spawnParams);
		if (!replacement)
			return;

		int slotId = slot.GetID();

		// Asked before anything is destroyed. If the slot would refuse the garment - wrong area
		// type, slot blocked by armour - the man keeps what the loadout gave him rather than
		// ending up bare because the roll was rejected halfway through.
		if (worn && !manager.CanReplaceItem(replacement, storage, slotId))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(replacement);
			return;
		}

		if (worn)
		{
			CPO_CarryPocketContents(manager, worn, replacement);
			manager.TryDeleteItem(worn);
		}

		if (manager.TryInsertItemInStorage(replacement, storage, slotId))
			return;

		Print("[7Cav PubFac] " + picked + " refused by " + SCR_ResourceNameUtils.GetPrefabName(owner) + ", that slot is now empty", LogLevel.WARNING);
		SCR_EntityHelper.DeleteEntityAndChildren(replacement);
	}

	//------------------------------------------------------------------------------------------------
	//! Moves everything out of the outgoing garment before it is deleted.
	//!
	//! Not cosmetic bookkeeping: the base kit routes map, compass and entrenching tool into the
	//! trousers and the medical items into the shirt through InitialInventoryItems, and every kit
	//! carries magazines and grenades in the vest pouches. Deleting a garment without emptying it
	//! first would quietly strip a squad of its kit. (An earlier version of this comment claimed
	//! the engine falls back to any storage when a TargetStorage does not fit; it does not, see
	//! docs/AI_GROUPS.md, "Routing rules".)
	protected void CPO_CarryPocketContents(notnull SCR_InventoryStorageManagerComponent manager, notnull IEntity from, notnull IEntity to)
	{
		array<Managed> sourceStorages = {};
		from.FindComponents(BaseInventoryStorageComponent, sourceStorages);
		if (sourceStorages.IsEmpty())
			return;

		array<Managed> targetStorages = {};
		to.FindComponents(BaseInventoryStorageComponent, targetStorages);

		foreach (int index, Managed sourceManaged : sourceStorages)
		{
			BaseInventoryStorageComponent source = BaseInventoryStorageComponent.Cast(sourceManaged);
			if (!source)
				continue;

			array<IEntity> carried = {};
			source.GetAll(carried, false);

			// Same index, so the second pocket of a pair of trousers lands in the second pocket
			// of the replacement rather than all of it piling into the first.
			BaseInventoryStorageComponent target;
			if (index < targetStorages.Count())
				target = BaseInventoryStorageComponent.Cast(targetStorages[index]);

			foreach (IEntity item : carried)
			{
				if (target && manager.TryMoveItemToStorage(item, target))
					continue;

				if (CPO_MoveAnywhereElse(manager, item, from))
					continue;

				Print("[7Cav PubFac] no room for " + SCR_ResourceNameUtils.GetPrefabName(item) + " during a uniform swap, it is lost with the old garment", LogLevel.WARNING);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Last resort for an item the replacement garment has no room for: any other storage the
	//! character carries, skipping the garment on its way out.
	protected bool CPO_MoveAnywhereElse(notnull SCR_InventoryStorageManagerComponent manager, notnull IEntity item, notnull IEntity leaving)
	{
		array<BaseInventoryStorageComponent> storages = {};
		manager.GetStorages(storages, EStoragePurpose.PURPOSE_DEPOSIT);

		foreach (BaseInventoryStorageComponent candidate : storages)
		{
			if (!candidate || candidate.GetOwner() == leaving)
				continue;

			if (manager.TryMoveItemToStorage(item, candidate))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! \return a prefab drawn from the pool in proportion to the entry weights, empty if the pool
	//! holds nothing usable
	protected ResourceName CPO_Pick(notnull array<ref CPO_UniformVariant> pool)
	{
		int total;
		foreach (CPO_UniformVariant variant : pool)
		{
			if (variant && variant.m_iWeight > 0 && !variant.m_sPrefab.IsEmpty())
				total += variant.m_iWeight;
		}

		if (total <= 0)
			return ResourceName.Empty;

		int roll = Math.RandomInt(0, total);

		foreach (CPO_UniformVariant variant : pool)
		{
			if (!variant || variant.m_iWeight <= 0 || variant.m_sPrefab.IsEmpty())
				continue;

			roll -= variant.m_iWeight;
			if (roll < 0)
				return variant.m_sPrefab;
		}

		return ResourceName.Empty;
	}
}
