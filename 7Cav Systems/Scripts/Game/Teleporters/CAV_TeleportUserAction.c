//------------------------------------------------------------------------------------------------
//! One destination slot on a teleporter pole. The prefab lists several of these with m_iSlot 0, 1,
//! 2 ... and slot N shows the Nth entry of CAV_TeleporterComponent.GetDestinations, so a pole offers
//! as many destinations as it has slots. Slot 0 also explains, greyed out, why a pole has nothing to
//! offer (arrival only, or no other teleporters).
//!
//! The action is local only: the travelling player's machine fades the screen and moves its own
//! character (see the component header for why the owner does the move).
//------------------------------------------------------------------------------------------------
class CAV_TeleportUserAction : ScriptedUserAction
{
	[Attribute("0", desc: "Which destination this entry shows: 0 = first in the pole's sorted list.", params: "0 63 1")]
	protected int m_iSlot;

	protected CAV_TeleporterComponent m_Teleporter;

	//! The destination this entry showed when the hold began. The list can change during the hold (a
	//! GM adds, removes or renames a pole), and the player must land where the label said.
	protected EntityID m_PickedDestinationID;
	protected bool m_bHasPick;

	protected static const string PREFIX = "Teleport: ";
	protected static const string NOTHING_HERE = "Teleport";

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Teleporter = CAV_TeleporterComponent.Cast(pOwnerEntity.FindComponent(CAV_TeleporterComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected CAV_TeleporterComponent GetSlotDestination()
	{
		if (!m_Teleporter)
			return null;

		return m_Teleporter.GetDestinationAt(m_iSlot);
	}

	//------------------------------------------------------------------------------------------------
	//! Slot 0 stays visible with a reason when the pole has nothing to offer.
	protected bool IsExplainerSlot()
	{
		if (m_iSlot != 0 || !m_Teleporter)
			return false;

		return !m_Teleporter.CanDepart() || !GetSlotDestination();
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_Teleporter)
			return false;

		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(user);
		if (!character || character.IsInVehicle())
			return false;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller || controller.GetLifeState() != ECharacterLifeState.ALIVE)
			return false;

		if (IsExplainerSlot())
			return true;

		return m_Teleporter.CanDepart() && GetSlotDestination() != null;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (!m_Teleporter)
			return false;

		if (!m_Teleporter.CanDepart())
		{
			SetCannotPerformReason("Arrival point only");
			return false;
		}

		if (!GetSlotDestination())
		{
			SetCannotPerformReason("No other teleporters");
			return false;
		}

		if (CAV_TeleporterComponent.IsLocalJumpInProgress())
		{
			SetCannotPerformReason("Travelling");
			return false;
		}

		float remaining = CAV_TeleporterComponent.GetLocalCooldownRemaining();
		if (remaining > 0)
		{
			int seconds = Math.Ceil(remaining);
			SetCannotPerformReason("Recharging " + seconds.ToString() + "s");
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		CAV_TeleporterComponent destination = GetSlotDestination();
		if (destination && m_Teleporter.CanDepart())
			outName = PREFIX + destination.GetDisplayName();
		else
			outName = NOTHING_HERE;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void OnActionStart(IEntity pUserEntity)
	{
		super.OnActionStart(pUserEntity);

		CAV_TeleporterComponent destination = GetSlotDestination();
		m_bHasPick = false;
		if (destination && destination.GetOwner())
		{
			m_PickedDestinationID = destination.GetOwner().GetID();
			m_bHasPick = true;
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.OnActionCanceled(pOwnerEntity, pUserEntity);
		m_bHasPick = false;
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		bool hadPick = m_bHasPick;
		m_bHasPick = false;

		if (!m_Teleporter || pUserEntity != SCR_PlayerController.GetLocalControlledEntity())
			return;

		if (!CanBePerformedScript(pUserEntity))
			return;

		CAV_TeleporterComponent destination;
		if (hadPick)
			destination = CAV_TeleporterComponent.FindByEntityID(m_PickedDestinationID);
		else
			destination = GetSlotDestination();

		if (destination && destination != m_Teleporter && destination.CanArrive())
			m_Teleporter.StartLocalJump(destination);
	}

	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false;
	}
}
