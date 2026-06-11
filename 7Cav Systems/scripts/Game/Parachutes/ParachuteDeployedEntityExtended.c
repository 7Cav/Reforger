class ParachuteDeployedEntityExtendedClass : ParachuteDeployedEntityClass {}
class ParachuteDeployedEntityExtended : ParachuteDeployedEntity
{
	[Attribute("250", UIWidgets.Slider, "Empty chute delete delay (ms)", "50 500 50", category : "Landing")]
	protected int m_iEmptyChuteDeleteDelayMs = 250;

	protected bool m_bDeployInvincibilityActive;

	bool IsDeployInvincibilityActive()
	{
		return m_bDeployInvincibilityActive;
	}

	bool IsPilotSeated()
	{
		return IronHorseParachuteFixes_Helper.IsEntityValid(m_Pilot)
			&& IronHorseParachuteFixes_Helper.IsPilotSeatedInChute(m_Pilot, this);
	}

	protected BaseCompartmentSlot GetCargoCompartmentSlotForDeploy()
	{
		if (m_Compartment && m_Compartment.GetType() == ECompartmentType.CARGO)
			return m_Compartment;

		return IronHorseParachuteFixes_Helper.FindCargoSlotOnEntity(this);
	}

	protected void EjectOccupantIfAny()
	{
		IronHorseParachuteFixes_Helper.EjectOccupantFromSlot(m_Compartment);

		BaseCompartmentSlot cargoSlot = GetCargoCompartmentSlotForDeploy();
		if (cargoSlot && cargoSlot != m_Compartment)
			IronHorseParachuteFixes_Helper.EjectOccupantFromSlot(cargoSlot);
	}

	//! Invincible until the jumper is seated — avoids chute collision kills during enter.
	void StartDeployInvincibilityUntilSeated()
	{
		if (!GetGame())
			return;

		m_bDeployInvincibilityActive = true;

		if (m_DamageManager)
			m_DamageManager.EnableDamageHandling(false);
	}

	void EndDeployInvincibilityWhenPilotSeated()
	{
		if (!m_bDeployInvincibilityActive)
			return;

		EndDeployInvincibility();
	}

	protected void EndDeployInvincibility()
	{
		m_bDeployInvincibilityActive = false;

		if (m_DamageManager)
			m_DamageManager.EnableDamageHandling(true);
	}

	override void SetPitch(float value = 0.0, EActionTrigger reason = 0, string actionName = string.Empty)
	{
		if (m_bDeployInvincibilityActive)
			return;

		super.SetPitch(value, reason, actionName);
	}

	override void SetRoll(float value = 0.0, EActionTrigger reason = 0, string actionName = string.Empty)
	{
		if (m_bDeployInvincibilityActive)
			return;

		super.SetRoll(value, reason, actionName);
	}

	override void DestroyParachute()
	{
		if (m_bIsDestroyed)
			return;

		m_bIsDestroyed = true;

		if (IsPilotSeated())
		{
			AskServerExit();
			// Do NOT delete here — Rpc_ServerExitParachute ejects the player, then deletes the chute.
		}
		else
		{
			EjectOccupantIfAny();
			if (GetGame())
				GetGame().GetCallqueue().CallLater(
					IronHorseParachuteFixes_Helper.DeleteEntityIfValid,
					m_iEmptyChuteDeleteDelayMs,
					false,
					this);
		}
	}
}
