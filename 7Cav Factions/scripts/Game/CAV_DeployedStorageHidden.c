//! A deployable radio pack whose storage is also its item (the Cav FILBE) loses the deployable item
//! component that hid it from pickup once deployed. Hide it the same way here, so a deployed radio
//! can only be taken back by dismantling it.
modded class SCR_UniversalInventoryStorageComponent
{
	override bool ShouldHideInVicinity()
	{
		SCR_BaseDeployableInventoryItemComponent deployable = SCR_BaseDeployableInventoryItemComponent.Cast(GetOwner().FindComponent(SCR_BaseDeployableInventoryItemComponent));
		if (deployable && deployable.IsDeployed())
			return true;

		return super.ShouldHideInVicinity();
	}
}
