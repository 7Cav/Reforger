//------------------------------------------------------------------------------------------------
class HMD_IffAttachableBeaconTurnOffAction : ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		HMD_IffAttachableBeaconComponent b = HMD_IffAttachableBeaconComponent.FindOnEntity(GetOwner());
		return b && b.IsBeaconActive();
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
			b.TrySetBeaconActive(false);
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = "Turn IFF beacon OFF";
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
