//------------------------------------------------------------------------------------------------
//! Admin-configurable arsenal lockouts, driven by $profile:7Cav_PubFac/arsenal.json.
//!
//! WHAT IT DOES. It hides chosen prefabs from this faction's arsenal list. The items stay in the
//! mod's generated catalog, so nothing has to be regenerated and nothing is deleted; a prefab that
//! is switched back on is simply offered again. It does NOT remove the assets from the world: a kit
//! or a script that equips one of these prefabs still does.
//!
//! THE SEAMS, and why there are two. The arsenal list a player sees is produced by
//! SCR_ArsenalComponent.GetFilteredArsenalItems, which does one of two things:
//!
//!   * with no overwrite item-list config set on the arsenal entity, it asks
//!     SCR_EntityCatalogManagerComponent.GetFilteredArsenalItems (SCR_ArsenalComponent.c:366), which
//!     builds its result array fresh and returns it, so removing from that array cannot disturb the
//!     catalog or its cache;
//!   * with an overwrite item-list config set, it asks that config instead (lines 381 and 384) and
//!     the catalog manager is never reached.
//!
//! Filtering only the manager would therefore miss every arsenal that uses an overwrite config, so
//! the override is applied at both: the manager, which is also what any other mod asking the same
//! question reads, and the arsenal component itself, which is the single place both of its own paths
//! pass through and what the arsenal display component reads (SCR_ArsenalDisplayComponent.c:77).
//! Filtering twice is harmless: the second pass finds the entry already gone. Both are scoped to
//! this faction, so another faction in the same world keeps what its own catalogs offer.
//!
//! THE FILE. `$profile:7Cav_PubFac/arsenal.json`, written on first use and re-read every 30 seconds
//! so an edit applies without a restart. Each entry is keyed by the prefab's own resource path and
//! carries `shown`, the same flag name the Frontline vehicle-costs file uses: 1 offers the item, 0
//! hides it, and any other value is rejected with a log line rather than read as an unlock. Sections
//! group by where the content comes from (USAF, ION, AFRF).
//!
//! READ BY NAME, ON PURPOSE. The reader walks the list shipped below rather than the file's own keys.
//! The serialization API could enumerate keys (SerializationContext.CanSeekMembers, and
//! LoadContext.ReadMapKey alongside StartMap), but a fixed list keeps the shipped defaults and the
//! owner's editable values in one place and makes every value in the file one this code actually
//! understands. The cost is that a prefab has to be listed below to be switchable: adding one is an
//! AddEntry line, not a file edit. Keys the file carries that this list does not know are ignored.
//!
//! FAIL BEHAVIOUR. A missing file is written from the shipped defaults. A file that exists but does
//! NOT parse runs on those defaults and is left exactly as it is, deliberately: rewriting it would
//! destroy an owner's half-finished edit, and the shipped defaults already hide what must stay
//! hidden. The defaults are the state wanted on a server nobody has configured.
//!
//! ONE THING TO KNOW BEFORE TRUSTING A LIVE EDIT. The list is built on the machine that opens the
//! arsenal, and the file lives in the profile directory of the machine that has one. On a listen
//! server and in single player that is the same machine, so an edit takes effect where it is made. On
//! a dedicated server the clients have no such file, fall back to the shipped defaults, and the
//! server's own edit does not travel: a lockout always holds everywhere, but switching something back
//! ON only reaches remote players once the setting is replicated, which is not implemented. The
//! Frontline vehicle-costs file broadcasts its deviations for exactly this reason.
class CPO_ArsenalFilterEntry
{
	string m_sSection;
	string m_sPrefab;
	string m_sName;
	int m_iShown;

	void CPO_ArsenalFilterEntry(string section, string prefab, string name, int shown)
	{
		m_sSection = section;
		m_sPrefab = prefab;
		m_sName = name;
		m_iShown = shown;
	}
}

class CPO_ArsenalFilter
{
	protected static ref CPO_ArsenalFilter s_pInstance;

	protected const string DIRECTORY = "$profile:7Cav_PubFac";
	protected const string FILE_PATH = "$profile:7Cav_PubFac/arsenal.json";
	//! Read by the two overrides below, so deliberately not protected.
	const string FACTION_KEY = "7Cav";
	protected const int SCHEMA = 1;
	protected const int RELOAD_MS = 30000;

	//! Every prefab this filter can act on, in section order, one row each.
	protected ref array<ref CPO_ArsenalFilterEntry> m_aEntries = {};

	//! Section order as it is written to the file, and the note each section carries.
	protected ref array<string> m_aSections = {};
	protected ref map<string, string> m_mSectionNote = new map<string, string>();

	//! The resolved answer, rebuilt from the entries: prefabs whose `shown` came back 0.
	protected ref set<string> m_Hidden = new set<string>();

	protected bool m_bBuilt;
	protected int m_iNextReload_ms;

	//------------------------------------------------------------------------------------------------
	static CPO_ArsenalFilter GetInstance()
	{
		if (!s_pInstance)
			s_pInstance = new CPO_ArsenalFilter();

		return s_pInstance;
	}

	//------------------------------------------------------------------------------------------------
	//! \return true when this prefab must not be offered in the arsenal
	bool IsHidden(ResourceName prefab)
	{
		EnsureFresh();

		if (m_Hidden.IsEmpty() || prefab.IsEmpty())
			return false;

		return m_Hidden.Contains(prefab);
	}

	//------------------------------------------------------------------------------------------------
	//! Drops every locked prefab from a list the arsenal is about to show. Used by the component
	//! seam; the manager seam runs the same test through IsHidden.
	void RemoveHidden(inout array<SCR_ArsenalItem> items)
	{
		EnsureFresh();

		if (m_Hidden.IsEmpty())
			return;

		for (int i = items.Count() - 1; i >= 0; i--)
		{
			SCR_ArsenalItem item = items[i];
			if (!item)
				continue;

			if (m_Hidden.Contains(item.GetItemResourceName()))
				items.RemoveOrdered(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Reads the file back at most once per RELOAD_MS, so a live edit lands without a restart.
	//!
	//! The clock is System.GetTickCount(), milliseconds since the game started, NOT
	//! BaseWorld.GetWorldTime(), which is the lifetime of the CURRENT world and restarts on every
	//! world load. With the world clock a reload would leave a deadline from the previous world in
	//! place, and the fresh world's smaller times would suppress re-reads for however long the old
	//! world had been running beyond the interval.
	protected void EnsureFresh()
	{
		BuildDefaults();

		int now = System.GetTickCount();
		if (m_iNextReload_ms != 0 && now < m_iNextReload_ms)
			return;

		m_iNextReload_ms = now + RELOAD_MS;

		Load();
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildDefaults()
	{
		if (m_bBuilt)
			return;

		m_bBuilt = true;

		AddSection("USAF", "RHS US content. The ball caps are listed because four of them show twice: RHS's '_backwards' prefabs are the forward one with a velcro offset and inherit its name, so hide those four here if the duplicated cards are unwanted.");
		AddSection("ION", "ION's content. Civilian clothing and the two Vz.58 rifles are hidden by default; the Vz.58 magazines, vest and pouch are listed so the whole family can be switched together.");
		AddSection("AFRF", "Reserved for OPFOR content. Empty on purpose: this build's catalog does not carry RHS's AFRF weapon, ammunition or attachment lists at all.");

		// --- USAF: offered by default, listed so they can be hidden without a code change.
		AddEntry("USAF", "Prefabs/Characters/HeadGear/Hat_BallCap/Hat_ballcap_MC.et", "Ball cap, Multicam", 1);
		AddEntry("USAF", "Prefabs/Characters/HeadGear/Hat_BallCap/Hat_ballcap_TAN.et", "Ball cap, tan", 1);
		AddEntry("USAF", "Prefabs/Characters/HeadGear/Hat_BallCap/Hat_ballcap_BLK.et", "Ball cap, black", 1);
		AddEntry("USAF", "Prefabs/Characters/HeadGear/Hat_BallCap/Hat_ballcap_GRY.et", "Ball cap, grey", 1);
		AddEntry("USAF", "Prefabs/Characters/HeadGear/Hat_BallCap/Hat_ballcap_OD.et", "Ball cap, olive drab", 1);
		AddEntry("USAF", "Prefabs/Characters/HeadGear/Hat_BallCap/Hat_ballcap_ATACS_backwards.et", "Ball cap, ATACS, backwards entry", 1);
		AddEntry("USAF", "Prefabs/Characters/HeadGear/Hat_BallCap/Hat_ballcap_BLK_backwards.et", "Ball cap, black, backwards entry", 1);
		AddEntry("USAF", "Prefabs/Characters/HeadGear/Hat_BallCap/Hat_ballcap_GRY_backwards.et", "Ball cap, grey, backwards entry", 1);
		AddEntry("USAF", "Prefabs/Characters/HeadGear/Hat_BallCap/Hat_ballcap_OD_backwards.et", "Ball cap, olive drab, backwards entry", 1);
		AddEntry("USAF", "Prefabs/Characters/HeadGear/Hat_BallCap/Hat_ballcap_TAN_backwards.et", "Ball cap, tan, backwards entry", 1);

		// --- ION: civilian clothing and the two Vz.58 rifles, hidden by default.
		AddEntry("ION", "Prefabs/Characters/Uniforms/Pants_Trousers_01/Pants_Trousers_01_LightBrown.et", "Trousers, light brown", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Pants_Trousers_01/Pants_Trousers_01_DarkGrey.et", "Trousers, dark grey", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Pants_Trousers_01/Pants_Trousers_01_DarkBrown.et", "Trousers, dark brown", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Shirt_CottonShirt_01/Shirt_CottonShirt_01_V1.et", "Cotton shirt 1", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Shirt_CottonShirt_01/Shirt_CottonShirt_01_V2.et", "Cotton shirt 2", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Shirt_CottonShirt_01/Shirt_CottonShirt_01_V3.et", "Cotton shirt 3", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Shirt_CottonShirt_01/Shirt_CottonShirt_01_V4.et", "Cotton shirt 4", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Shirt_CottonShirt_01/Shirt_CottonShirt_01_V5.et", "Cotton shirt 5", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Shirt_CottonShirt_01/Shirt_CottonShirt_01_V6.et", "Cotton shirt 6", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Shirt_Turtleneck_01/Shirt_Turtleneck_01_grey.et", "Turtleneck, grey", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Shirt_Turtleneck_01/Shirt_Turtleneck_01_DarkBlue.et", "Turtleneck, dark blue", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Shirt_Turtleneck_01/Shirt_Turtleneck_01_beige.et", "Turtleneck, beige", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/T_Shirts/ION_Shirt/ION_Casual_Shirt_Brand.et", "ION casual shirt, branded", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/T_Shirts/ION_Shirt/ION_Casual_Shirt_Tan.et", "ION casual shirt, tan", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Shirt_flannel/flannel_shirt_blue_1.et", "Flannel shirt, blue", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Shirt_flannel/flannel_shirt_brown_1.et", "Flannel shirt, brown", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Shirt_flannel/flannel_shirt_green_1.et", "Flannel shirt, green", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Shirt_flannel/flannel_shirt_red_1.et", "Flannel shirt, red", 0);
		AddEntry("ION", "Prefabs/Characters/Uniforms/Shirt_flannel/flannel_shirt_white_1.et", "Flannel shirt, white", 0);
		AddEntry("ION", "Prefabs/Weapons/Rifles/VZ58/Rifle_VZ58P.et", "Vz.58 P", 0);
		AddEntry("ION", "Prefabs/Weapons/Rifles/VZ58/Rifle_VZ58V.et", "Vz.58 V", 0);

		// --- ION: the rest of the Vz.58 family, offered, so it can follow the rifles if wanted.
		AddEntry("ION", "Prefabs/Weapons/Magazines/Vz58/Magazine_762x39_Vz58_30rnd_Ball.et", "Vz.58 magazine, ball", 1);
		AddEntry("ION", "Prefabs/Weapons/Magazines/Vz58/Magazine_762x39_Vz58_30rnd_Tracer.et", "Vz.58 magazine, tracer", 1);
		AddEntry("ION", "Prefabs/Weapons/Magazines/Vz58/Magazine_762x39_Vz58_30rnd_Last_5Tracer.et", "Vz.58 magazine, last five tracer", 1);
		AddEntry("ION", "Prefabs/Characters/Vests/Vest_JPC/variants/Vest_JPC_RGR_nobelt_VZ58.et", "JPC vest, ranger green, Vz.58", 1);
		AddEntry("ION", "Prefabs/Characters/Vests/Vest_JPC_Pouches/Pouch_jpc_placard_RGR_VZ58.et", "JPC placard pouch, ranger green, Vz.58", 1);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddSection(string section, string note)
	{
		m_aSections.Insert(section);
		m_mSectionNote.Set(section, note);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddEntry(string section, string prefab, string name, int shown)
	{
		m_aEntries.Insert(new CPO_ArsenalFilterEntry(section, prefab, name, shown));
	}

	//------------------------------------------------------------------------------------------------
	//! Merge the file over the shipped defaults and rebuild the hidden set. Values the file carries
	//! win; prefabs the file is missing keep their shipped state and are written out on the next save,
	//! so an older file picks up a newly listed item without the owner redoing anything.
	protected void Load()
	{
		bool haveFile = FileIO.FileExists(FILE_PATH);
		bool parsed = false;

		JsonLoadContext loadContext;
		if (haveFile)
		{
			loadContext = new JsonLoadContext();
			parsed = loadContext.LoadFromFile(FILE_PATH);

			if (!parsed)
				Print("[" + CPO_FactionsInfo.NAME + "] arsenal filter: " + FILE_PATH + " exists but did not parse, running on the shipped defaults and leaving the file untouched", LogLevel.WARNING);
		}

		if (parsed)
			ReadEntries(loadContext);

		RebuildHidden();

		Print("[" + CPO_FactionsInfo.NAME + "] arsenal filter: " + m_Hidden.Count().ToString() + " prefab(s) hidden from the arsenal, " + m_aEntries.Count().ToString() + " configurable in " + FILE_PATH, LogLevel.NORMAL);

		// Write only when there was nothing to protect. A first run creates the file; a good parse
		// refreshes it with any newly listed prefab. A file that failed to parse is left exactly as
		// it is, because the alternative is overwriting an owner's edit with the shipped defaults and
		// losing every change they had made.
		if (!haveFile || parsed)
			Save();
	}

	//------------------------------------------------------------------------------------------------
	//! Reads each listed prefab by name inside its section. Anything the file carries that is not
	//! listed below is ignored, which is why the key set is fixed by code.
	protected void ReadEntries(JsonLoadContext loadContext)
	{
		string openSection;

		foreach (CPO_ArsenalFilterEntry entry : m_aEntries)
		{
			if (entry.m_sSection != openSection)
			{
				if (openSection != string.Empty)
					loadContext.EndObject();

				openSection = string.Empty;

				if (loadContext.StartObject(entry.m_sSection))
					openSection = entry.m_sSection;
			}

			if (openSection == string.Empty)
				continue;

			if (!loadContext.StartObject(entry.m_sPrefab))
				continue;

			int shown = entry.m_iShown;
			bool read = loadContext.ReadValue("shown", shown);

			loadContext.EndObject();

			// Only 1 and 0 mean anything. A read that failed leaves the shipped default in place, and
			// so does a value outside the pair: a typo must not widen a default restriction.
			if (!read)
				continue;

			if (shown == 0 || shown == 1)
			{
				entry.m_iShown = shown;
				continue;
			}

			Print("[" + CPO_FactionsInfo.NAME + "] arsenal filter: ignoring shown " + shown.ToString() + " for " + entry.m_sPrefab + ", keeping the shipped default", LogLevel.WARNING);
		}

		if (openSection != string.Empty)
			loadContext.EndObject();
	}

	//------------------------------------------------------------------------------------------------
	protected void RebuildHidden()
	{
		m_Hidden.Clear();

		foreach (CPO_ArsenalFilterEntry entry : m_aEntries)
		{
			if (entry.m_iShown == 0)
				m_Hidden.Insert(entry.m_sPrefab);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Writes the merged state back, so the file always carries every configurable prefab. Sections
	//! are walked rather than the entries, so a section with nothing in it still appears with its note
	//! instead of silently vanishing from the file.
	protected void Save()
	{
		if (!FileIO.FileExists(DIRECTORY))
			FileIO.MakeDirectory(DIRECTORY);

		PrettyJsonSaveContext saveContext = new PrettyJsonSaveContext();
		saveContext.SetIndent(" ", 2);

		saveContext.WriteValue("schema", SCHEMA);
		saveContext.WriteValue("readme", "Arsenal lockouts for the 7Cav faction. Each entry is keyed by the prefab's resource path: shown 1 offers it in the arsenal, shown 0 hides it, and any other value is ignored in favour of the built-in default. Hidden prefabs stay in the mod, so switching one back is just this file. Edits are read again within 30 seconds and apply the next time the arsenal is opened. The sections group by where the content comes from and are otherwise only for reading: a prefab can be switched, a section cannot. Only the prefabs listed here can be switched, so ask for an item to be added to the list if it should be controllable. The prefab paths and this file's shape are rewritten by the mod; the shown values are yours.");

		foreach (string section : m_aSections)
		{
			saveContext.StartObject(section);

			foreach (CPO_ArsenalFilterEntry entry : m_aEntries)
			{
				if (entry.m_sSection != section)
					continue;

				saveContext.StartObject(entry.m_sPrefab);
				saveContext.WriteValue("name", entry.m_sName);
				saveContext.WriteValue("shown", entry.m_iShown);
				saveContext.EndObject();
			}

			// The note goes in last, after the prefab keys, so a human reading the file meets the
			// entries first. The reader is not order-sensitive; this is for readability only.
			string note = m_mSectionNote.Get(section);
			if (note != string.Empty && note != "0")
				saveContext.WriteValue("_note", note);

			saveContext.EndObject();
		}

		if (!saveContext.SaveToFile(FILE_PATH))
			Print("[" + CPO_FactionsInfo.NAME + "] arsenal filter: failed to write " + FILE_PATH, LogLevel.ERROR);
	}
}

//------------------------------------------------------------------------------------------------
//! Seam one: the catalog manager. SCR_ArsenalComponent asks this for its list whenever the arsenal
//! entity has no overwrite item-list config, and any other mod asking the same question reads it too.
modded class SCR_EntityCatalogManagerComponent
{
	override array<SCR_ArsenalItem> GetFilteredArsenalItems(SCR_EArsenalItemType typeFilter, SCR_EArsenalItemMode modeFilter, SCR_EArsenalGameModeType arsenalGameModeType, SCR_Faction faction = null, EArsenalItemDisplayType requiresDisplayType = -1)
	{
		array<SCR_ArsenalItem> items = super.GetFilteredArsenalItems(typeFilter, modeFilter, arsenalGameModeType, faction, requiresDisplayType);

		// Scope: this faction only. A world where another mod's faction shares the arsenal manager
		// must keep that faction's own catalog intact.
		if (!faction || faction.GetFactionKey() != CPO_ArsenalFilter.FACTION_KEY)
			return items;

		CPO_ArsenalFilter filter = CPO_ArsenalFilter.GetInstance();
		if (!filter)
			return items;

		for (int i = items.Count() - 1; i >= 0; i--)
		{
			SCR_ArsenalItem item = items[i];
			if (!item)
				continue;

			if (filter.IsHidden(item.GetItemResourceName()))
				items.RemoveOrdered(i);
		}

		return items;
	}
}

//------------------------------------------------------------------------------------------------
//! Seam two: the arsenal component itself, which is the one place both of its own paths pass
//! through. This is what covers an arsenal entity that carries an overwrite item-list config, a request
//! that never reaches the catalog manager above, and it is also what the arsenal display component
//! reads. Filtering here as well as above is harmless: the second pass finds the entry already gone.
modded class SCR_ArsenalComponent
{
	override bool GetFilteredArsenalItems(out notnull array<SCR_ArsenalItem> filteredArsenalItems, EArsenalItemDisplayType requiresDisplayType = -1)
	{
		bool ok = super.GetFilteredArsenalItems(filteredArsenalItems, requiresDisplayType);
		if (!ok)
			return ok;

		SCR_Faction faction = GetAssignedFaction();
		if (!faction || faction.GetFactionKey() != CPO_ArsenalFilter.FACTION_KEY)
			return ok;

		CPO_ArsenalFilter filter = CPO_ArsenalFilter.GetInstance();
		if (filter)
			filter.RemoveHidden(filteredArsenalItems);

		return ok;
	}
}
