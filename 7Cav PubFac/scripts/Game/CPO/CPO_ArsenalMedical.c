//------------------------------------------------------------------------------------------------
//! Stocks 7Cav's equipment arsenal boxes with medical supplies.
//!
//! Vanilla splits the buildable arsenal into typed boxes: the weapons box lists RIFLE, PISTOL,
//! MACHINE_GUN and SNIPER_RIFLE, the equipment box lists the clothing and gear types, and HEAL
//! items live only in the dedicated medical crate and the unrestricted supply point arsenal. A
//! player who builds the two-box arsenal therefore has no medical supplies at their outpost at
//! all. The supported-type mask is per component instance and every path that matters (init,
//! faction change, mode toggles) funnels through RefreshArsenal, whose own RPC and JIP stream
//! replicate the current masks, so widening the mask right before the vanilla body runs is
//! authoritative and reaches clients with no extra plumbing. It also needs no prefab overrides:
//! any equipment-family box (clothing types present, HEAL absent) affiliated to 7Cav gains
//! HEAL, whichever mod's prefab delivered it. The weapons box lacks the clothing signature and
//! other factions fail the key check, so both stay exactly vanilla.
modded class SCR_ArsenalComponent
{
	//------------------------------------------------------------------------------------------------
	override void RefreshArsenal(bool init = false, SCR_Faction faction = null)
	{
		CPO_AddMedicalToEquipmentBox();
		super.RefreshArsenal(init, faction);
	}

	//------------------------------------------------------------------------------------------------
	protected void CPO_AddMedicalToEquipmentBox()
	{
		if (!(m_eSupportedArsenalItemTypes & SCR_EArsenalItemType.TORSO))
			return;

		if (m_eSupportedArsenalItemTypes & SCR_EArsenalItemType.HEAL)
			return;

		// Vehicle-mounted arsenals (Vehicle_ArsenalBox_Base) also carry the clothing types
		// without HEAL, but vehicles affiliate to whoever crews them, so a widen there would
		// stick after a 7Cav player leaves and leak into another faction's view. Their mask
		// carries the weapon types; the pure equipment boxes never do, so RIFLE is the guard.
		if (m_eSupportedArsenalItemTypes & SCR_EArsenalItemType.RIFLE)
			return;

		SCR_Faction assignedFaction = GetAssignedFaction();
		if (!assignedFaction || assignedFaction.GetFactionKey() != "7Cav")
			return;

		m_eSupportedArsenalItemTypes = m_eSupportedArsenalItemTypes | SCR_EArsenalItemType.HEAL;
		Print("[" + CPO_FactionsInfo.NAME + "] equipment arsenal box widened with medical supplies for 7Cav", LogLevel.NORMAL);
	}
}
