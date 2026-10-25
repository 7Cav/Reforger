//------------------------------------------------------------------------------------------------
//! Offers deployed radios and command trucks to friendly factions, not only their own. The server
//! does not check faction when reserving a spawn point; only these lists do. Other spawn points are
//! unchanged.
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
		return GetSpawnPointsForFaction(factionKey).Count();
	}
}

//------------------------------------------------------------------------------------------------
//! The deploy map keeps its own exact faction match when icons are added or change faction.
modded class SCR_MapUIElementContainer
{
	//------------------------------------------------------------------------------------------------
	override protected void AddSpawnPoint(SCR_SpawnPoint spawnPoint)
	{
		if (!spawnPoint.IsSpawnPointVisibleForPlayer(SCR_PlayerController.GetLocalPlayerId()))
			return;

		Faction playerFaction = SCR_FactionManager.SGetLocalPlayerFaction();
		if (playerFaction && SCR_SpawnPoint.CAV_IsOfferedToFaction(spawnPoint, playerFaction.GetFactionKey()))
		{
			ShowSpawnPoint(spawnPoint);
			UpdateIcons();
		}
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnSpawnPointFactionChange(SCR_SpawnPoint spawnPoint)
	{
		Faction playerFaction = m_PlyFactionAffilComp.GetAffiliatedFaction();
		if (playerFaction && SCR_SpawnPoint.CAV_IsOfferedToFaction(spawnPoint, playerFaction.GetFactionKey()))
			AddSpawnPoint(spawnPoint);
		else
			RemoveSpawnPoint(spawnPoint);
	}
}

//------------------------------------------------------------------------------------------------
//! End the deploy menu's wait for a first spawn point when a friendly radio or truck appears.
modded class SCR_PlayerDeployMenuHandlerComponent
{
	//------------------------------------------------------------------------------------------------
	override protected void OnPlayerSpawnPointAdded(SCR_SpawnPoint sp)
	{
		if (!SCR_SpawnPoint.CAV_IsOfferedToFaction(sp, m_PlyFactionAffil.GetAffiliatedFactionKey()))
			return;

		SCR_SpawnPoint.Event_SpawnPointAdded.Remove(OnPlayerSpawnPointAdded);

		m_bReady = true;
		GetGame().GetCallqueue().Call(SetReadyDelayed);
	}
}

//------------------------------------------------------------------------------------------------
//! The deploy menu's spawn list keeps its own exact faction match on add and faction change.
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
