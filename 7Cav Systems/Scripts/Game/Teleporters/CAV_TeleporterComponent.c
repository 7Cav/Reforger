//------------------------------------------------------------------------------------------------
//! Teleporters (7Cav/Reforger#10): a Game Master placeable pole. A player at one pole picks any other
//! pole from its interaction list and is moved there. Every pole is both origin and destination unless
//! the GM sets its Direction to departure only or arrival only.
//!
//! Who does what:
//! - The server owns the settings (name preset, direction). They are RplProps, so every client and
//!   join-in-progress player holds the same values, and the GM attribute writes land on the server.
//! - The RplComponent is not streamable (as on vanilla spawn points), so every client knows every
//!   pole however far away it is, and the destination list is the same everywhere.
//! - The move itself runs on the travelling player's machine, through SCR_Global.TeleportLocalPlayer.
//!   The engine makes the owner authoritative over a character's position (vanilla GM teleports and
//!   Conflict fast travel both end on the owner), so the server cannot move a player by itself.
//!
//! Names: index 0 is automatic (nearest map location plus grid, the lookup vanilla GM tasks use). The
//! other indices are presets from GetPresetNames. Keep that list append-only (never reorder or remove
//! an entry): GM saves and World Editor placements store the index.
//!
//! GM saves keep Name and Direction through CAV_TeleporterSerializer.c.
//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "7Cav/Teleporters", description: "Makes this entity a teleporter. Needs a non-streamable RplComponent and CAV_TeleportUserAction slots in its ActionsManagerComponent.")]
class CAV_TeleporterComponentClass : ScriptComponentClass
{
}

//------------------------------------------------------------------------------------------------
enum CAV_ETeleporterDirection
{
	BOTH,
	DEPARTURE_ONLY,
	ARRIVAL_ONLY
}

//------------------------------------------------------------------------------------------------
class CAV_TeleporterComponent : ScriptComponent
{
	[Attribute("0", desc: "Name preset index. 0 = automatic (nearest map location and grid); see CAV_TeleporterComponent.GetPresetNames."), RplProp(onRplName: "OnSettingsChanged")]
	protected int m_iNameIndex;

	[Attribute("0", UIWidgets.ComboBox, "Which way players may travel through this teleporter.", enums: ParamEnumArray.FromEnum(CAV_ETeleporterDirection)), RplProp(onRplName: "OnSettingsChanged")]
	protected CAV_ETeleporterDirection m_eDirection;

	[Attribute("10", desc: "Seconds a player must wait after a jump before using any teleporter again.", params: "0 600 1")]
	protected float m_fCooldown;

	[Attribute("2.5", desc: "Arrival point distance in front of the pole (metres, along the pole's facing).", params: "0.5 20 0.1")]
	protected float m_fArrivalOffset;

	[Attribute("1000", desc: "A map location further away than this is not used in the automatic name (metres).", params: "0 10000 10")]
	protected float m_fLocationRange;

	//! Every live teleporter on this machine. Plain (weak) entries: the entity owns the component.
	protected static ref array<CAV_TeleporterComponent> s_aAll = {};

	//! World time (ms) before which the local player may not jump again.
	protected static float s_fLocalCooldownUntil;

	//! The world the cooldown and jump flags belong to. Statics outlive a world (Workbench play
	//! sessions, mission changes), and world time restarts with each one.
	protected static BaseWorld s_StateWorld;
	protected static float s_fJumpStartedAt;

	//! Display name cache; rebuilt when the pole moves or its preset changes.
	protected string m_sCachedName;
	protected vector m_vCachedNamePos;
	protected int m_iCachedNameIndex = -1;

	protected static ref array<string> s_aPresetNames;

	protected ref array<CAV_TeleporterComponent> m_aDestinationCache;
	protected float m_fDestinationCacheTime = -1;

	protected static const float NAME_CACHE_MOVE_TOLERANCE_SQ = 1;
	protected static const float FADE_OUT_SECONDS = 0.6;
	protected static const float BLACK_SECONDS = 0.4;
	protected static const float FADE_IN_SECONDS = 0.8;
	protected static const float ARRIVAL_SEARCH_RADIUS = 6;
	protected static const float JUMP_STALE_MS = 5000;

	//! The pending jump's destination, by entity ID so a pole deleted during the fade resolves to null.
	protected static EntityID s_PendingDestinationID;
	protected static bool s_bJumpInProgress;

	//------------------------------------------------------------------------------------------------
	//! Preset names for the GM Name dropdown. Index 0 is the automatic name. APPEND ONLY (see header).
	static array<string> GetPresetNames()
	{
		if (!s_aPresetNames)
		{
			s_aPresetNames = {
				"Automatic (nearest location)",
				"Main Base", "FOB", "COP", "Patrol Base", "OP", "LZ", "HLZ", "Rally Point", "Airfield", "Port",
				"Alpha", "Bravo", "Charlie", "Delta", "Echo", "Foxtrot", "Golf", "Hotel", "India", "Juliett",
				"Kilo", "Lima", "Mike", "November", "Oscar", "Papa", "Quebec", "Romeo", "Sierra", "Tango",
				"Uniform", "Victor", "Whiskey", "X-ray", "Yankee", "Zulu"
			};
		}

		return s_aPresetNames;
	}

	//------------------------------------------------------------------------------------------------
	int GetNameIndex()
	{
		return m_iNameIndex;
	}

	//------------------------------------------------------------------------------------------------
	CAV_ETeleporterDirection GetDirection()
	{
		return m_eDirection;
	}

	//------------------------------------------------------------------------------------------------
	//! Server only (the GM attribute write lands here).
	void SetNameIndex(int index)
	{
		if (!GetPresetNames().IsIndexValid(index) || index == m_iNameIndex)
			return;

		m_iNameIndex = index;
		Replication.BumpMe();
		OnSettingsChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! Server only (the GM attribute write lands here).
	void SetDirection(CAV_ETeleporterDirection direction)
	{
		if (direction < CAV_ETeleporterDirection.BOTH || direction > CAV_ETeleporterDirection.ARRIVAL_ONLY || direction == m_eDirection)
			return;

		m_eDirection = direction;
		Replication.BumpMe();
		OnSettingsChanged();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSettingsChanged()
	{
		m_iCachedNameIndex = -1;
	}

	//------------------------------------------------------------------------------------------------
	bool CanDepart()
	{
		return m_eDirection != CAV_ETeleporterDirection.ARRIVAL_ONLY;
	}

	//------------------------------------------------------------------------------------------------
	bool CanArrive()
	{
		return m_eDirection != CAV_ETeleporterDirection.DEPARTURE_ONLY;
	}

	//------------------------------------------------------------------------------------------------
	//! Name shown in other poles' destination lists, e.g. "FOB (Morton)" or "Morton 045 112".
	string GetDisplayName()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return string.Empty;

		vector pos = owner.GetOrigin();
		if (m_iCachedNameIndex == m_iNameIndex && vector.DistanceSq(pos, m_vCachedNamePos) < NAME_CACHE_MOVE_TOLERANCE_SQ)
			return m_sCachedName;

		string location = FindLocationName(pos);
		string grid = FormatGrid(pos);
		string name;

		array<string> presets = GetPresetNames();
		if (m_iNameIndex > 0 && presets.IsIndexValid(m_iNameIndex))
		{
			// The grid keeps two presets near one town apart ("FOB (Morton 045 112)").
			name = presets[m_iNameIndex];
			if (!location.IsEmpty())
				name += " (" + location + " " + grid + ")";
			else
				name += " (" + grid + ")";
		}
		else if (!location.IsEmpty())
		{
			name = location + " " + grid;
		}
		else
		{
			name = "Grid " + grid;
		}

		m_sCachedName = name;
		m_vCachedNamePos = pos;
		m_iCachedNameIndex = m_iNameIndex;
		return name;
	}

	//------------------------------------------------------------------------------------------------
	//! Nearest named map location within m_fLocationRange, translated, or empty.
	protected string FindLocationName(vector pos)
	{
		SCR_EditableEntityCore core = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		if (!core)
			return string.Empty;

		SCR_EditableEntityComponent nearest = core.FindNearestEntity(pos, EEditableEntityType.COMMENT, EEditableEntityFlag.LOCAL);
		if (!nearest)
			return string.Empty;

		GenericEntity locationEntity = nearest.GetOwner();
		if (!locationEntity)
			return string.Empty;

		if (vector.DistanceXZ(locationEntity.GetOrigin(), pos) > m_fLocationRange)
			return string.Empty;

		string name;
		SCR_MapDescriptorComponent descriptor = SCR_MapDescriptorComponent.Cast(locationEntity.FindComponent(SCR_MapDescriptorComponent));
		if (descriptor && descriptor.Item())
			name = descriptor.Item().GetDisplayName();

		if (name.IsEmpty() && nearest.GetInfo())
			name = nearest.GetInfo().GetName();

		if (name.IsEmpty())
			return string.Empty;

		name = WidgetManager.Translate(name);
		return name.Trim();
	}

	//------------------------------------------------------------------------------------------------
	//! Six-figure grid as on the map, e.g. "045 112".
	protected static string FormatGrid(vector pos)
	{
		int gridX, gridZ;
		SCR_MapEntity.GetGridPos(pos, gridX, gridZ, 2, 4);
		return gridX.ToString(3) + " " + gridZ.ToString(3);
	}

	//------------------------------------------------------------------------------------------------
	//! Every other teleporter a player at this one may travel to, sorted by display name. The order is
	//! what the interaction slots index, and it is rebuilt on every call, so a slot always means the
	//! entry it was showing a moment ago.
	int GetDestinations(notnull array<CAV_TeleporterComponent> outDestinations)
	{
		outDestinations.Clear();

		IEntity owner = GetOwner();
		if (!owner)
			return 0;

		BaseWorld world = owner.GetWorld();
		array<string> names = {};

		for (int i = s_aAll.Count() - 1; i >= 0; i--)
		{
			CAV_TeleporterComponent other = s_aAll[i];
			if (!other || !other.GetOwner())
			{
				s_aAll.Remove(i);
				continue;
			}

			if (other == this || !other.CanArrive() || other.GetOwner().GetWorld() != world)
				continue;

			// Insertion sort by name: the list is short (one entry per pole on the map).
			string name = other.GetDisplayName();
			int insertAt = names.Count();
			for (int j = 0; j < names.Count(); j++)
			{
				if (name.Compare(names[j], false) < 0)
				{
					insertAt = j;
					break;
				}
			}

			names.InsertAt(name, insertAt);
			outDestinations.InsertAt(other, insertAt);
		}

		return outDestinations.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! Destination for interaction slot `slot`, or null. The sorted list is rebuilt at most once per
	//! frame: every slot asks several times a frame while the player looks at the pole.
	CAV_TeleporterComponent GetDestinationAt(int slot)
	{
		BaseWorld world = GetGame().GetWorld();
		float now = -1;
		if (world)
			now = world.GetWorldTime();

		if (!m_aDestinationCache || now != m_fDestinationCacheTime)
		{
			if (!m_aDestinationCache)
				m_aDestinationCache = {};

			GetDestinations(m_aDestinationCache);
			m_fDestinationCacheTime = now;
		}

		if (!m_aDestinationCache.IsIndexValid(slot))
			return null;

		return m_aDestinationCache[slot];
	}

	//------------------------------------------------------------------------------------------------
	//! A clear standing spot in front of this pole. Falls back to the offset point itself.
	vector FindArrivalPosition()
	{
		IEntity owner = GetOwner();
		vector transform[4];
		owner.GetWorldTransform(transform);

		vector forward = transform[2];
		forward[1] = 0;
		if (forward.LengthSq() < 0.01)
			forward = vector.Forward;

		forward.Normalize();

		// Start the search above the pole's foot: FindEmptyTerrainPosition traces down to the first
		// surface, so a pole on a roof or a deck lands the player on that roof or deck.
		vector center = transform[3] + forward * m_fArrivalOffset;
		center[1] = center[1] + 1.5;

		vector found;
		if (SCR_WorldTools.FindEmptyTerrainPosition(found, center, ARRIVAL_SEARCH_RADIUS))
			return found;

		center[1] = Math.Max(transform[3][1], owner.GetWorld().GetSurfaceY(center[0], center[2]));
		return center;
	}

	//------------------------------------------------------------------------------------------------
	float GetCooldown()
	{
		return m_fCooldown;
	}

	//------------------------------------------------------------------------------------------------
	//! Seconds before the local player may jump again (0 when ready).
	static float GetLocalCooldownRemaining()
	{
		ChimeraWorld world = GetGame().GetWorld();
		if (!world || world != s_StateWorld)
			return 0;

		float remaining = (s_fLocalCooldownUntil - world.GetWorldTime()) * 0.001;
		if (remaining < 0)
			return 0;

		return remaining;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsLocalJumpInProgress()
	{
		if (!s_bJumpInProgress)
			return false;

		// A jump whose call never ran (world unloaded, call queue cleared) must not block forever.
		BaseWorld world = GetGame().GetWorld();
		if (!world || world != s_StateWorld || world.GetWorldTime() - s_fJumpStartedAt > JUMP_STALE_MS)
		{
			s_bJumpInProgress = false;
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! The live teleporter on the entity with this ID, or null.
	static CAV_TeleporterComponent FindByEntityID(EntityID id)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		IEntity entity = world.FindEntityByID(id);
		if (!entity)
			return null;

		return CAV_TeleporterComponent.Cast(entity.FindComponent(CAV_TeleporterComponent));
	}

	//------------------------------------------------------------------------------------------------
	//! Start the local player's jump to destination: fade out, move, fade in. Local machine only.
	void StartLocalJump(notnull CAV_TeleporterComponent destination)
	{
		if (IsLocalJumpInProgress() || !destination.GetOwner())
			return;

		ChimeraWorld world = GetGame().GetWorld();
		if (!world)
			return;

		s_bJumpInProgress = true;
		s_StateWorld = world;
		s_fJumpStartedAt = world.GetWorldTime();
		s_PendingDestinationID = destination.GetOwner().GetID();
		s_fLocalCooldownUntil = world.GetWorldTime() + (FADE_OUT_SECONDS + BLACK_SECONDS + m_fCooldown) * 1000;

		Fade(true, FADE_OUT_SECONDS);
		GetGame().GetCallqueue().CallLater(CAV_TeleporterComponent.FinishLocalJump, (FADE_OUT_SECONDS + BLACK_SECONDS) * 1000);
	}

	//------------------------------------------------------------------------------------------------
	//! Public only so the call queue can reach it; not an API.
	static void FinishLocalJump()
	{
		s_bJumpInProgress = false;

		CAV_TeleporterComponent destination = FindByEntityID(s_PendingDestinationID);
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());

		// Re-check at the moment of the move: the pole may have been deleted or switched to departure
		// only during the fade, and the player may have died or got into a vehicle.
		bool moved;
		if (destination && destination.CanArrive() && character && !character.IsInVehicle())
		{
			CharacterControllerComponent controller = character.GetCharacterController();
			if (controller && controller.GetLifeState() == ECharacterLifeState.ALIVE)
				moved = SCR_Global.TeleportLocalPlayer(destination.FindArrivalPosition(), SCR_EPlayerTeleportedReason.FAST_TRAVEL);
		}

		Fade(false, FADE_IN_SECONDS);

		// A jump that did not happen costs no cooldown, and the player is told why nothing moved.
		if (!moved)
		{
			s_fLocalCooldownUntil = 0;
			if (character)
				SCR_HintManagerComponent.ShowCustomHint("The destination teleporter was removed or no longer accepts arrivals.", "Teleport cancelled", 5);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void Fade(bool toBlack, float seconds)
	{
		SCR_ScreenEffectsManager manager = SCR_ScreenEffectsManager.GetScreenEffectsDisplay();
		if (!manager)
			return;

		SCR_FadeInOutEffect fade = SCR_FadeInOutEffect.Cast(manager.GetEffect(SCR_FadeInOutEffect));
		if (fade)
			fade.FadeOutEffect(toBlack, seconds);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (!s_aAll.Contains(this))
			s_aAll.Insert(this);

		// A teleporter knocked down by fire or a vehicle would silently drop out of every list, so the
		// pole takes no damage. Damage is server-authoritative; clients follow.
		if (Replication.IsServer())
		{
			DamageManagerComponent damage = DamageManagerComponent.Cast(owner.FindComponent(DamageManagerComponent));
			if (damage)
				damage.EnableDamageHandling(false);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		s_aAll.RemoveItem(this);
		super.OnDelete(owner);
	}
}
