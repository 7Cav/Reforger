//------------------------------------------------------------------------------------------------
class HMD_IffAttachableBeaconTurnOnAction : ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		HMD_IffAttachableBeaconComponent b = HMD_IffAttachableBeaconComponent.FindOnEntity(GetOwner());
		return b && b.CanConfigure();
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		HMD_IffAttachableBeaconComponent b = HMD_IffAttachableBeaconComponent.FindOnEntity(pOwnerEntity);
		if (b)
			b.TrySetBeaconActive(true);
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		HMD_IffAttachableBeaconComponent b = HMD_IffAttachableBeaconComponent.FindOnEntity(GetOwner());
		if (!b)
		{
			outName = "Turn IFF beacon ON";
			return true;
		}
		outName = string.Format("Turn IFF beacon ON (%1)", b.GetPreviewLabel());
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return true;
	}
}
