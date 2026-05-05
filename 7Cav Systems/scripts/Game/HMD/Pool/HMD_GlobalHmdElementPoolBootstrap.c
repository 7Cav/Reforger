//------------------------------------------------------------------------------------------------
//! Spawns a replicated pool root under SCR_BaseGameMode when none exists (no editor-placed pool required).
class HMD_GlobalHmdElementPoolBootstrap
{
	static const ResourceName HMD_POOL_ROOT_PREFAB = "{6900000100003001}Prefabs/Systems/HMD/HMD_GlobalHmdElementPoolRoot.et";

	//------------------------------------------------------------------------------------------------
	static void ServerEnsurePoolExists(SCR_BaseGameMode gm)
	{
		if (!gm || !Replication.IsServer())
			return;
		if (HMD_GlobalHmdElementPoolComponent.FindPool())
			return;
		IEntity root = gm;
		if (!root)
			return;
		BaseWorld world = root.GetWorld();
		if (!world)
			return;
		Resource res = Resource.Load(HMD_POOL_ROOT_PREFAB);
		if (!res || !res.IsValid())
			return;
		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		gm.GetWorldTransform(sp.Transform);
		IEntity spawned = GetGame().SpawnEntityPrefab(res, world, sp);
		if (!spawned)
			return;
		root.AddChild(spawned, -1);
		vector localMat[4];
		localMat[0] = vector.Right;
		localMat[1] = vector.Up;
		localMat[2] = vector.Forward;
		localMat[3] = vector.Zero;
		spawned.SetLocalTransform(localMat);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_BaseGameMode
{
	//------------------------------------------------------------------------------------------------
	protected void HmdResetClientVisualizationSession()
	{
		HMD_ElementHudBridge.ResetForNewPlaySession();
		SCR_HUDManagerComponent.ResetPoolMirrorHandshakeForNewPlaySession();
		HMD_ElementVisualizationRegistry.ResetForNewPlaySession();
		HMD_ElementVisualizationInput.ResetForNewPlaySession();
		HMD_LaserDesignatorReadoutHud.ResetForNewPlaySession();
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (pool)
			pool.ServerClearMirrorSubscriptionForPlayer(playerId);
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameModeStart()
	{
		HmdResetClientVisualizationSession();
		super.OnGameModeStart();
		HMD_GlobalHmdElementPoolBootstrap.ServerEnsurePoolExists(this);
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameModeEnd(SCR_GameModeEndData endData)
	{
		super.OnGameModeEnd(endData);
		HmdResetClientVisualizationSession();
	}
}
