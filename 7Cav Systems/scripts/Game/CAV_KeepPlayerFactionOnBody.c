//! Keep a player's body on the player's faction whenever the game reports a spawn. Vanilla stamps the
//! faction when a loadout spawns, but a mod that swaps the body afterwards (Bacon Loadout Editor) spawns
//! a fresh prefab and keeps that prefab's own faction. Every Cav body inherits Character_US_Base, so the
//! swapped body reads "US": Cav radios then refuse to deploy, and Bacon files the next loadout under US.
modded class SCR_BaseGameMode
{
	override protected void OnGameStart()
	{
		super.OnGameStart();

		if (Replication.IsServer())
			GetOnPlayerSpawned().Insert(CAV_OnPlayerSpawned);
	}

	//! A possess spawn (reconnecting into the old body) fires the spawned event before vanilla moves the
	//! player onto the body's faction, so wait a frame and read the player's faction once that has run.
	protected void CAV_OnPlayerSpawned(int playerId, IEntity player)
	{
		GetGame().GetCallqueue().CallLater(CAV_StampPlayerFaction, 0, false, playerId, player);
	}

	protected void CAV_StampPlayerFaction(int playerId, IEntity player)
	{
		if (!player)
			return;

		Faction playerFaction = SCR_FactionManager.SGetPlayerFaction(playerId);
		if (!playerFaction)
			return;

		FactionAffiliationComponent aff = FactionAffiliationComponent.Cast(player.FindComponent(FactionAffiliationComponent));
		if (aff && aff.GetAffiliatedFaction() != playerFaction)
			aff.SetAffiliatedFactionByKey(playerFaction.GetFactionKey());
	}
}
