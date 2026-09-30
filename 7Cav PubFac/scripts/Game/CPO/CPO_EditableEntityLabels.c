//------------------------------------------------------------------------------------------------
//! Faction label for 7Cav, so 7Cav is its own entry in the Game Master content
//! browser's faction filter and its own build-menu faction, the way RHS_USAF is.
//!
//! Every faction gets exactly one label (SCR_Faction.m_FactionLabel). 7Cav used to inherit
//! FACTION_US from US.conf, so the browser filed its soldiers and groups under US Army and had no
//! filter of its own, and the RHS_USAF filter picked them up too through the label inherited
//! from Character_RHS_USAF_Base.
//!
//! The value has to be unique across every mod that extends this enum, since they all merge into
//! one enum at load. Vanilla uses 0-300, DCO 72xx, RHS 105xx and a few hashes, KSC 13001, CRX
//! 7007xx. This is "CPOU" as ASCII, 0x43504F55, checked against every enum extension in the
//! installed addon set on 2026-08-17.
//!
//! The filter button itself is Configs/Core/EditableEntityCore.conf (a delta of vanilla's, RHS
//! does the same), and the faction picks the label up in Configs/Factions/7Cav.conf.
modded enum EEditableEntityLabel
{
	FACTION_CPO_US = 1129270101,
}
