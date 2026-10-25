//------------------------------------------------------------------------------------------------
//! Opens deployed radios and command trucks to friendly factions.
//! Vanilla only lists a spawn point for players of exactly its own faction, so a Misfit player
//! could never spawn on a Bandit radio even though the factions are friendly. The server does not
//! check faction when reserving a spawn point; the restriction lives in which points the deploy
//! menu and the map offer. These overrides widen those lists for deployed radios
//! (SCR_RestrictedDeployableSpawnPoint) and command trucks (SCR_RadioVehicleSpawnPoint) only, so
//! base and Game Master placed spawn points stay with their own faction.
modded class SCR_SpawnPoint
{
	//------------------------------------------------------------------------------------------------
	//! True if a player of factionKey may be offered this spawn point.
	static bool CAV_IsOfferedToFaction(SCR_SpawnPoint spawnPoint, string factionKey)
	{
		if (!spawnPoint || factionKey.IsEmpty())
			return false;

		string pointKey = spawnPoint.GetFactionKey();
		if (pointKey == factionKey)
			return true;

		if (pointKey.IsEmpty())
			return false;

		if (!SCR_RestrictedDeployableSpawnPoint.Cast(spawnPoint) && !SCR_RadioVehicleSpawnPoint.Cast(spawnPoint))
			return false;

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return false;

		Faction pointFaction = factionManager.GetFactionByKey(pointKey);
		Faction playerFaction = factionManager.GetFactionByKey(factionKey);
		if (!pointFaction || !playerFaction)
			return false;

		return pointFaction.IsFactionFriendly(playerFaction);
	}

	//------------------------------------------------------------------------------------------------
	override static array<SCR_SpawnPoint> GetSpawnPointsForFaction(string factionKey)
	{
		array<SCR_SpawnPoint> factionSpawnPoints = new array<SCR_SpawnPoint>();
		if (factionKey.IsEmpty())
			return factionSpawnPoints;

		foreach (SCR_SpawnPoint spawnPoint : GetSpawnPoints())
		{
			if (CAV_IsOfferedToFaction(spawnPoint, factionKey))
				factionSpawnPoints.Insert(spawnPoint);
		}

		return factionSpawnPoints;
	}

	//------------------------------------------------------------------------------------------------
	override static int GetSpawnPointCountForFaction(string factionKey)
	{
		if (factionKey.IsEmpty())
			return 0;

		int count = 0;
		foreach (SCR_SpawnPoint spawnPoint : GetSpawnPoints())
		{
			if (CAV_IsOfferedToFaction(spawnPoint, factionKey))
				count++;
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	override static void ShowSpawnPointDescriptors(bool show, Faction faction)
	{
		string factionKey;
		if (faction)
			factionKey = faction.GetFactionKey();

		foreach (SCR_SpawnPoint spawnPoint : GetSpawnPoints())
		{
			if (!spawnPoint)
				continue;

			SCR_MapDescriptorComponent mapDescriptor = SCR_MapDescriptorComponent.Cast(spawnPoint.FindComponent(SCR_MapDescriptorComponent));
			if (!mapDescriptor || !mapDescriptor.Item())
				continue;

			mapDescriptor.Item().SetVisible(show && CAV_IsOfferedToFaction(spawnPoint, factionKey));
		}
	}
}

//------------------------------------------------------------------------------------------------
//! The deploy menu adds and removes points as they appear or change faction using an exact key
//! match of its own; route both through the same rule.
modded class SCR_SpawnPointRequestUIComponent
{
	//------------------------------------------------------------------------------------------------
	override protected void OnSpawnPointAdded(SCR_SpawnPoint spawnPoint)
	{
		Faction playerFaction = SCR_FactionManager.SGetLocalPlayerFaction();
		if (playerFaction && SCR_SpawnPoint.CAV_IsOfferedToFaction(spawnPoint, playerFaction.GetFactionKey()))
			AddSpawnPoint(spawnPoint);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnSpawnPointFactionChange(SCR_SpawnPoint spawnPoint)
	{
		Faction playerFaction = SCR_FactionManager.SGetLocalPlayerFaction();
		if (playerFaction && SCR_SpawnPoint.CAV_IsOfferedToFaction(spawnPoint, playerFaction.GetFactionKey()))
			AddSpawnPoint(spawnPoint);
		else
			RemoveSpawnPoint(spawnPoint);
	}
}
