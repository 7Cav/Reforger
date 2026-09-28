//------------------------------------------------------------------------------------------------
//! Keeps foreign command trucks out of the build-mode content browser in 7Cav worlds.
//!
//! The depot build menu is not fed by the depot faction's catalog alone.
//! SCR_PlaceableEntitiesRegistryFromCatalog builds the browser's card list by unioning EVERY
//! registered faction's vehicle catalog (GetFilteredEditorPrefabsOfAllFactions: entries carrying
//! enabled SCR_EntityCatalogEditorData valid in BUILDING mode, deduplicated by prefab), and the
//! cards are then only label-filtered per provider. The vanilla US faction still lists the
//! woodland M923A1 command truck with BUILDING editor data, its FACTION_US label is admitted at
//! 7Cav providers (under COE2 the parent deliberately admits FACTION_US so depots keep vanilla
//! vehicles), and so its card kept appearing at the vanilla budget cost of 420 next to ours no
//! matter how clean 7Cav's own catalog was. A per-faction conf hide can never reach another
//! faction's catalog instance, and the registry caches its prefab list in its constructor, so the
//! reliable seam is this query, which every registry build funnels through regardless of timing.
//!
//! Scope is deliberately narrow: BUILDING mode, vehicle catalogs, and only when 7Cav is
//! registered in the world. Game Master's EDIT registry is untouched, so a GM can still place the
//! vanilla truck from the US faction tab; a depot build menu offers exactly one command truck per
//! livery, ours.
modded class SCR_EntityCatalogManagerComponent
{
	//------------------------------------------------------------------------------------------------
	override int GetFilteredEditorPrefabs(EEntityCatalogType catalogType, EEditorMode editorMode, SCR_Faction faction, notnull out array<ResourceName> filteredPrefabsList, array<EEditableEntityLabel> includedLabels = null, array<EEditableEntityLabel> excludedLabels = null, bool needsAllIncludedLabels = true)
	{
		int count = super.GetFilteredEditorPrefabs(catalogType, editorMode, faction, filteredPrefabsList, includedLabels, excludedLabels, needsAllIncludedLabels);

		if (editorMode != EEditorMode.BUILDING || catalogType != EEntityCatalogType.VEHICLE)
			return count;

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager || !factionManager.GetFactionByKey("7Cav"))
			return count;

		for (int i = filteredPrefabsList.Count() - 1; i >= 0; i--)
		{
			string prefab = filteredPrefabsList[i];
			if (!prefab.Contains("M923A1_command") || prefab.Contains("_CPO"))
				continue;

			string source = "factionless";
			if (faction)
				source = faction.GetFactionKey();

			Print("[" + CPO_FactionsInfo.NAME + "] build menu: dropped foreign command truck " + prefab + " (delivered by " + source + ")", LogLevel.NORMAL);
			filteredPrefabsList.RemoveOrdered(i);
		}

		return filteredPrefabsList.Count();
	}
}
