//------------------------------------------------------------------------------------------------
//! Game Master attributes for teleporters: Name (preset dropdown) and Direction. The rows only appear
//! when the selected entity carries a CAV_TeleporterComponent (ReadVariable returns null otherwise).
//!
//! Registration is done from script, not by overriding the shared EditorModeEdit.et prefab: that
//! prefab is winner-take-all between mods (Enfusion Modding KB section 2). The list is appended BEFORE
//! super because vanilla's init loop sets up every attribute it finds, and both init paths are hooked:
//! the client draws the panel from its copy, the server applies the write to its own.
//------------------------------------------------------------------------------------------------
class CAV_TeleporterAttr
{
	//! The teleporter behind a selected editable entity, or null.
	static CAV_TeleporterComponent Get(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;

		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;

		return CAV_TeleporterComponent.Cast(owner.FindComponent(CAV_TeleporterComponent));
	}
}

//------------------------------------------------------------------------------------------------
//! Name preset. Entries come from CAV_TeleporterComponent.GetPresetNames, the one list both this
//! dropdown and the destination names read, so the conf holds no copy of them.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class CAV_TeleporterNameEditorAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	protected void CAV_BuildEntries()
	{
		if (!m_aValues)
			m_aValues = new array<ref SCR_EditorAttributeFloatStringValueHolder>();

		array<string> presets = CAV_TeleporterComponent.GetPresetNames();
		if (m_aValues.Count() == presets.Count())
			return;

		m_aValues.Clear();
		foreach (int i, string preset : presets)
		{
			SCR_EditorAttributeFloatStringValueHolder holder = new SCR_EditorAttributeFloatStringValueHolder();
			holder.SetName(preset);
			holder.SetFloatValue(i);
			m_aValues.Insert(holder);
		}
	}

	//------------------------------------------------------------------------------------------------
	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		CAV_BuildEntries();
		return super.GetEntries(outEntries);
	}

	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CAV_TeleporterComponent teleporter = CAV_TeleporterAttr.Get(item);
		if (!teleporter)
			return null;

		CAV_BuildEntries();
		int index = ConvertValueToIndex(teleporter.GetNameIndex());
		if (index < 0)
			index = 0;

		return SCR_BaseEditorAttributeVar.CreateInt(index);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		CAV_TeleporterComponent teleporter = CAV_TeleporterAttr.Get(item);
		if (!teleporter)
			return;

		CAV_BuildEntries();
		float value;
		if (!ConvertIndexToValue(var.GetInt(), value))
			return;

		int nameIndex = Math.Round(value);
		teleporter.SetNameIndex(nameIndex);
	}
}

//------------------------------------------------------------------------------------------------
//! Direction: two-way, departure only, arrival only. Values (the CAV_ETeleporterDirection numbers)
//! and labels are in the attribute list conf.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class CAV_TeleporterDirectionEditorAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CAV_TeleporterComponent teleporter = CAV_TeleporterAttr.Get(item);
		if (!teleporter)
			return null;

		int index = ConvertValueToIndex(teleporter.GetDirection());
		if (index < 0)
			index = 0;

		return SCR_BaseEditorAttributeVar.CreateInt(index);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		CAV_TeleporterComponent teleporter = CAV_TeleporterAttr.Get(item);
		if (!teleporter)
			return;

		float value;
		if (!ConvertIndexToValue(var.GetInt(), value))
			return;

		int direction = Math.Round(value);
		teleporter.SetDirection(direction);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_AttributesManagerEditorComponentClass
{
	//! Guards against a second append if an editor mode is initialised more than once.
	protected bool m_bCAV_TeleporterAttributesAppended;

	//------------------------------------------------------------------------------------------------
	void CAV_AppendTeleporterAttributes(notnull SCR_EditorAttributeList list)
	{
		if (m_bCAV_TeleporterAttributesAppended)
			return;

		m_bCAV_TeleporterAttributesAppended = true;
		list.InsertAllAttributes(m_aAttributes);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_AttributesManagerEditorComponent
{
	protected static const ResourceName CAV_TELEPORTER_ATTRIBUTES = "{7C1A5E30D2B84F61}Configs/Editor/AttributeLists/CAV_Teleporter_Attributes.conf";

	protected static ref SCR_EditorAttributeList s_CAV_TeleporterAttributes;

	//------------------------------------------------------------------------------------------------
	override void EOnEditorInit()
	{
		CAV_RegisterTeleporterAttributes();
		super.EOnEditorInit();
	}

	//------------------------------------------------------------------------------------------------
	override void EOnEditorInitServer()
	{
		CAV_RegisterTeleporterAttributes();
		super.EOnEditorInitServer();
	}

	//------------------------------------------------------------------------------------------------
	protected void CAV_RegisterTeleporterAttributes()
	{
		SCR_AttributesManagerEditorComponentClass data = SCR_AttributesManagerEditorComponentClass.Cast(GetEditorComponentData());
		if (!data)
			return;

		if (!s_CAV_TeleporterAttributes)
			s_CAV_TeleporterAttributes = CAV_LoadTeleporterAttributes();

		if (s_CAV_TeleporterAttributes)
			data.CAV_AppendTeleporterAttributes(s_CAV_TeleporterAttributes);
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_EditorAttributeList CAV_LoadTeleporterAttributes()
	{
		Resource holder = Resource.Load(CAV_TELEPORTER_ATTRIBUTES);
		if (!holder || !holder.IsValid())
		{
			Print("[CAV Teleporters] could not load " + CAV_TELEPORTER_ATTRIBUTES + "; the teleporter GM attributes will be missing", LogLevel.WARNING);
			return null;
		}

		BaseResourceObject res = holder.GetResource();
		if (!res)
			return null;

		SCR_EditorAttributeList list = SCR_EditorAttributeList.Cast(BaseContainerTools.CreateInstanceFromContainer(res.ToBaseContainer()));
		if (!list)
			Print("[CAV Teleporters] " + CAV_TELEPORTER_ATTRIBUTES + " did not resolve to an SCR_EditorAttributeList", LogLevel.WARNING);

		return list;
	}
}
