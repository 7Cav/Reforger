//------------------------------------------------------------------------------------------------
//! Global HMD helmet policy: Configs/HMD/VehicleHelmetPrefabs.conf (prefab list + enforce flag) plus optional HMD_HelmetCapabilityComponent on character attachments (LaserFixesAgain / HUDMarkerSystem parity).
class HMD_HmdHelmetPolicy
{
	//------------------------------------------------------------------------------------------------
	protected static void CollectCharacterAttachmentEntities(IEntity root, int depth, int maxDepth, notnull array<IEntity> outEnts)
	{
		if (!root || depth > maxDepth)
			return;
		outEnts.Insert(root);
		IEntity child = root.GetChildren();
		while (child)
		{
			CollectCharacterAttachmentEntities(child, depth + 1, maxDepth, outEnts);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! True if any entity under the character has HMD_HelmetCapabilityComponent (typically helmet attachment).
	static bool CharacterHasHelmetCapabilityTag(SCR_ChimeraCharacter ch)
	{
		if (!ch)
			return false;
		array<IEntity> ents = {};
		CollectCharacterAttachmentEntities(ch, 0, 12, ents);
		for (int i = 0; i < ents.Count(); i++)
		{
			IEntity e = ents[i];
			if (!e)
				continue;
			if (HMD_HelmetCapabilityComponent.Cast(e.FindComponent(HMD_HelmetCapabilityComponent)))
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! True if any attachment prefab ResourceName matches the configured list (exact or IsKindOf, same as loader).
	static bool CharacterHelmetMatchesGlobalPrefabList(SCR_ChimeraCharacter ch)
	{
		if (!ch)
			return false;
		array<IEntity> ents = {};
		CollectCharacterAttachmentEntities(ch, 0, 12, ents);
		for (int i = 0; i < ents.Count(); i++)
		{
			IEntity e = ents[i];
			if (!e)
				continue;
			if (HMD_VehicleHelmetPrefabsLoader.EntityPrefabMatchesHelmetList(e, ResourceName.Empty))
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Wearing HMD helmet: capability tag on an attachment, or prefab path matches global list when the list is non-empty.
	static bool CharacterHasHmdHelmetCapability(SCR_ChimeraCharacter ch)
	{
		if (!ch)
			return false;
		if (CharacterHasHelmetCapabilityTag(ch))
			return true;
		if (HMD_VehicleHelmetPrefabsLoader.HasGlobalHelmetPrefabList() && CharacterHelmetMatchesGlobalPrefabList(ch))
			return true;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! True when global helmet policy applies and the character does not qualify as HMD (Laser VehicleHudShouldRestrictToCameraOnly shape, without HUDMarkerSystem).
	static bool VehicleHudShouldRestrictToCameraOnlyForCharacter(SCR_ChimeraCharacter ch)
	{
		if (!ch || !ch.IsInVehicle())
			return false;
		if (!HMD_VehicleHelmetPrefabsLoader.HasGlobalHelmetPrefabList() && !HMD_VehicleHelmetPrefabsLoader.GetGlobalEnforceHmdHelmetInVehicles())
			return false;
		return !CharacterHasHmdHelmetCapability(ch);
	}
}
