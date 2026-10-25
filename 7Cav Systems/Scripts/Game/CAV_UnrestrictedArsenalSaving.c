//------------------------------------------------------------------------------------------------
//! Lets players save any kit they are wearing at any 7Cav arsenal.
//! Vanilla arsenals default to FACTION_ITEMS_ONLY, which refuses the whole save if a single
//! item is missing from the faction's item catalog, and which save type a box has depended on
//! which mod's copy of the box prefab loaded last. Every box that allows saving is promoted to
//! NO_RESTRICTIONS before vanilla init runs, on the server and on clients alike, so the value is
//! the same everywhere without an RPC. Boxes authored with saving disabled stay disabled, and a
//! Game Master can still change any box at runtime. The loadout save blacklist still applies.
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
