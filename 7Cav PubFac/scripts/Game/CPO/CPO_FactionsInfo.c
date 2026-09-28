//------------------------------------------------------------------------------------------------
//! Build stamp for this mod.
//!
//! VERSION is hand-maintained and must be bumped in step with the workshop publish, alongside the
//! version heading in the changelog. Nothing reads the addon.gproj at runtime, so this constant is
//! the only place the running build can name itself.
class CPO_FactionsInfo
{
	static const string NAME = "7Cav PubFac";
	static const string VERSION = "1.0.0";
}

//------------------------------------------------------------------------------------------------
//! Announces the loaded build once, at world init, so a server log opens with the version rather
//! than leaving you to infer it from behaviour. Every other line this mod prints carries the same
//! "[7Cav PubFac]" prefix, so one search finds the version and everything that followed it.
//!
//! Hung off SCR_BaseGameMode rather than a specific game mode so it fires whatever the world is
//! running: COE2, Conflict, plain Game Master or the Workbench play button. EOnInit is used rather
//! than OnGameModeStart because the latter fires on a replicated state change, which a client
//! joining a running server never sees.
modded class SCR_BaseGameMode
{
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		Print("[" + CPO_FactionsInfo.NAME + "] v" + CPO_FactionsInfo.VERSION + " loaded", LogLevel.NORMAL);

		// The catalog dump waits until the faction manager has certainly finished building
		// catalogs; asking a faction for a catalog before then is an engine error, not a null.
		GetGame().GetCallqueue().CallLater(CPO_LogFactionCatalogs, 5000, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Un-requests every command truck that is not ours from this faction's depots, whatever
	//! catalog delivered it. The conf-side per-faction hides cover the entry IDs known at build
	//! time (vanilla's two, RHS's re-list), but which entries actually reach the merged catalog
	//! depends on the server's modlist (WCS_Arsenal, for one, retargets the vanilla vehicle
	//! catalog reference to its own copy), so this guard works on the RESULT: any resolved entry
	//! whose prefab names M923A1_command and is not the _CPO variant loses its spawner data.
	//! SetEnabled on the data is the engine's sanctioned runtime switch for exactly this, the
	//! entry stays in Game Master, and catalogs are per-faction instances, so only Task Force
	//! Dagger is touched. Runs on every machine: catalogs are built locally from confs, not
	//! replicated, and the request menu reads the client's copy.
	//! The depot BUILD menu is a different pipeline entirely (the editor content browser; see
	//! CPO_BuildBrowserFilter.c). This guard covers the request/spawner path only.
	protected void CPO_UnrequestForeignCommandTrucks(notnull SCR_Faction faction)
	{
		SCR_EntityCatalog catalog = faction.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE, false);
		if (!catalog)
			return;

		array<SCR_EntityCatalogEntry> entries = {};
		catalog.GetEntityList(entries);
		foreach (SCR_EntityCatalogEntry entry : entries)
		{
			string prefab = entry.GetPrefab();
			if (!prefab.Contains("M923A1_command") || prefab.Contains("_CPO"))
				continue;

			SCR_EntityCatalogSpawnerData spawnerData = SCR_EntityCatalogSpawnerData.Cast(entry.GetEntityDataOfType(SCR_EntityCatalogSpawnerData));
			if (!spawnerData)
				continue;

			spawnerData.SetEnabled(false);
			Print("[" + CPO_FactionsInfo.NAME + "] Un-requested foreign command truck: " + prefab, LogLevel.NORMAL);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Writes what 7Cav actually resolved, catalog by catalog, and lists every vehicle by name.
	//!
	//! Exists because "is the catalog applying?" cost a full afternoon of source simulation once.
	//! The vehicle list is what a depot draws from, so comparing this line against a depot's menu
	//! separates "the entry is not in the faction" from "the depot is filtering it out", which are
	//! different bugs with different owners.
	protected void CPO_LogFactionCatalogs()
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return;

		SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey("7Cav"));
		if (!faction)
		{
			Print("[" + CPO_FactionsInfo.NAME + "] 7Cav is not registered in this world's faction manager", LogLevel.WARNING);
			return;
		}

		// Before the log pass, so the printed vehicle list shows the result as (gm-only).
		CPO_UnrequestForeignCommandTrucks(faction);

		string summary = "[" + CPO_FactionsInfo.NAME + "] 7Cav catalogs:";
		array<EEntityCatalogType> types = {EEntityCatalogType.CHARACTER, EEntityCatalogType.GROUP, EEntityCatalogType.VEHICLE, EEntityCatalogType.ITEM};
		foreach (EEntityCatalogType type : types)
		{
			SCR_EntityCatalog catalog = faction.GetFactionEntityCatalogOfType(type, false);
			int count = 0;
			if (catalog)
			{
				array<SCR_EntityCatalogEntry> entries = {};
				count = catalog.GetEntityList(entries);
			}
			summary += " " + typename.EnumToString(EEntityCatalogType, type) + "=" + count;
		}
		Print(summary, LogLevel.NORMAL);

		SCR_EntityCatalog vehicles = faction.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE, false);
		if (!vehicles)
			return;

		array<SCR_EntityCatalogEntry> list = {};
		vehicles.GetEntityList(list);

		// Twenty names to a line keeps each Print well inside anything the logger might truncate.
		string line;
		int onLine = 0;
		foreach (SCR_EntityCatalogEntry entry : list)
		{
			string prefab = entry.GetPrefab();
			int slash = prefab.LastIndexOf("/");
			if (slash >= 0)
				prefab = prefab.Substring(slash + 1, prefab.Length() - slash - 1);

			bool spawnable = entry.GetEntityDataOfType(SCR_EntityCatalogSpawnerData) != null;
			if (!spawnable)
				prefab += "(gm-only)";

			if (onLine > 0)
				line += ", ";
			line += prefab;
			onLine++;

			if (onLine == 20)
			{
				Print("[" + CPO_FactionsInfo.NAME + "] vehicles: " + line, LogLevel.NORMAL);
				line = "";
				onLine = 0;
			}
		}
		if (onLine > 0)
			Print("[" + CPO_FactionsInfo.NAME + "] vehicles: " + line, LogLevel.NORMAL);

		// Ground truth for the command truck cards, swept across EVERY registered faction: the
		// build menu's content browser unions all factions' vehicle catalogs and dedupes by
		// prefab, so a foreign card can come from a faction this mod never touched. For each
		// command truck entry this prints who delivers it, the resolved display name and supply
		// cost, and whether it is valid in BUILDING mode, exactly as the menus receive them.
		// Added when a card presenting the wrong name and price cost a day of hunting ghosts.
		array<Faction> allFactions = {};
		factionManager.GetFactionsList(allFactions);
		foreach (Faction sweepFaction : allFactions)
		{
			SCR_Faction scrSweepFaction = SCR_Faction.Cast(sweepFaction);
			if (!scrSweepFaction)
				continue;

			SCR_EntityCatalog sweepCatalog = scrSweepFaction.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE, false);
			if (!sweepCatalog)
				continue;

			array<SCR_EntityCatalogEntry> sweepEntries = {};
			sweepCatalog.GetEntityList(sweepEntries);
			foreach (SCR_EntityCatalogEntry catalogEntry : sweepEntries)
			{
				string prefabPath = catalogEntry.GetPrefab();
				if (!prefabPath.Contains("M923A1_command"))
					continue;

				int cost = -1;
				SCR_EntityCatalogSpawnerData entrySpawnerData = SCR_EntityCatalogSpawnerData.Cast(catalogEntry.GetEntityDataOfType(SCR_EntityCatalogSpawnerData));
				if (entrySpawnerData)
					cost = entrySpawnerData.GetSupplyCost();

				string displayName = "<no ui info>";
				SCR_UIInfo info = catalogEntry.GetEntityUiInfo();
				if (info)
					displayName = info.GetName();

				string buildingTag = "no";
				SCR_EntityCatalogEditorData editorData = SCR_EntityCatalogEditorData.Cast(catalogEntry.GetEntityDataOfType(SCR_EntityCatalogEditorData));
				if (editorData && editorData.IsValidInEditorMode(EEditorMode.BUILDING))
					buildingTag = "yes";

				string factionKey = scrSweepFaction.GetFactionKey();
				Print("[" + CPO_FactionsInfo.NAME + "] command truck entry [" + factionKey + "]: " + prefabPath + " | name '" + displayName + "' | cost " + cost.ToString() + " | building " + buildingTag, LogLevel.NORMAL);
			}
		}
	}
}
