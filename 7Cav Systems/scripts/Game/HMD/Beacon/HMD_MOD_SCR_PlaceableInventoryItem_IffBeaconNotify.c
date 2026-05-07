//------------------------------------------------------------------------------------------------
//! After placement, notify `HMD_IffAttachableBeaconComponent` on the same entity (IFF beacon prefab only).
//! `PlacementDone` and `OnPlacedOnGround` can both fire; beacon debounces then tells the server to set placed state + pool (no prefab reposition).
modded class SCR_PlaceableInventoryItemComponent : SCR_PlaceableInventoryItemComponent
{
	//------------------------------------------------------------------------------------------------
	protected static void HMD_TryNotifyIffAttachableBeaconPlaced(IEntity owner)
	{
		if (!owner)
			return;
		HMD_IffAttachableBeaconComponent b = HMD_IffAttachableBeaconComponent.FindOnEntity(owner);
		if (b)
			b.ScheduleNotifyPlacedInWorld();
	}

	//------------------------------------------------------------------------------------------------
	override void PlacementDone(notnull ChimeraCharacter user)
	{
		super.PlacementDone(user);
		HMD_TryNotifyIffAttachableBeaconPlaced(GetOwner());
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlacedOnGround()
	{
		super.OnPlacedOnGround();
		HMD_TryNotifyIffAttachableBeaconPlaced(GetOwner());
	}
}
