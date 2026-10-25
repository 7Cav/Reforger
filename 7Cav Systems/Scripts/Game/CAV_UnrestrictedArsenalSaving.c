//------------------------------------------------------------------------------------------------
//! Lets players save any kit they are wearing at any arsenal. Boxes that allow saving are promoted to
//! NO_RESTRICTIONS before vanilla init, identically on server and clients. Boxes with saving disabled
//! stay disabled, a GM can still change any box, and the loadout save blacklist still applies.
modded class SCR_ArsenalComponent
{
	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		if (!SCR_Global.IsEditMode() && m_eArsenalSaveType != SCR_EArsenalSaveType.SAVING_DISABLED)
			m_eArsenalSaveType = SCR_EArsenalSaveType.NO_RESTRICTIONS;

		super.OnPostInit(owner);
	}
}
