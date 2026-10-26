//------------------------------------------------------------------------------------------------
//! Keeps a teleporter's Name and Direction in Game Master saves.
//!
//! GM saves go through the PersistenceSystem, which only stores component state for serializers
//! listed in a persistence config, and the config cannot be extended from script. Every GM-placed
//! entity already passes through SCR_EditableEntityComponentSerializer (vanilla's EditableEntity.conf
//! rule), so the teleporter settings ride on it. Game Master Enhanced mods the same class the same
//! way: the previous layer's data goes into a named sub-object and ours sits beside it under unique
//! names, so the two nest cleanly whichever loads first.
//!
//! Only teleporters get the wrapper. Every other entity is written exactly as before, and a save
//! without the wrapper (any older save) loads through the unchanged path. A teleporter always writes
//! all three fields: in the binary save format reads are positional, so a layer that wrote nothing
//! would let the next layer's reads run on into someone else's data.
//------------------------------------------------------------------------------------------------
modded class SCR_EditableEntityComponentSerializer
{
	protected static const string CAV_TELEPORTER_BASE = "cavTeleporterBase";

	//------------------------------------------------------------------------------------------------
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull SaveContext context)
	{
		CAV_TeleporterComponent teleporter = CAV_TeleporterComponent.Cast(owner.FindComponent(CAV_TeleporterComponent));
		if (!teleporter)
			return super.Serialize(owner, component, context);

		context.StartObject(CAV_TELEPORTER_BASE);
		ESerializeResult baseResult = super.Serialize(owner, component, context);
		context.EndObject();

		if (baseResult == ESerializeResult.ERROR)
			return ESerializeResult.ERROR;

		int nameIndex = teleporter.GetNameIndex();
		int direction = teleporter.GetDirection();

		context.WriteValue("cavTeleporterVersion", 1);
		context.WriteValue("cavTeleporterName", nameIndex);
		context.WriteValue("cavTeleporterDirection", direction);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull LoadContext context)
	{
		// Same shape as Game Master Enhanced: unwrap when the wrapper is there, and look for our
		// fields either way (a save format that drops empty objects loses the wrapper, not the fields).
		if (context.DoesObjectExist(CAV_TELEPORTER_BASE))
		{
			if (!context.StartObject(CAV_TELEPORTER_BASE) || !super.Deserialize(owner, component, context) || !context.EndObject())
				return false;
		}
		else if (!super.Deserialize(owner, component, context))
		{
			return false;
		}

		CAV_TeleporterComponent teleporter = CAV_TeleporterComponent.Cast(owner.FindComponent(CAV_TeleporterComponent));
		if (!teleporter)
			return true;

		int version;
		if (!context.ReadValue("cavTeleporterVersion", version))
			return true;

		int nameIndex;
		int direction;
		context.ReadValueDefault("cavTeleporterName", nameIndex, 0);
		context.ReadValueDefault("cavTeleporterDirection", direction, 0);

		teleporter.SetNameIndex(nameIndex);
		teleporter.SetDirection(direction);
		return true;
	}
}
