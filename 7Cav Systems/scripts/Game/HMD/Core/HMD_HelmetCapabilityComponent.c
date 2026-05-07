//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "HUD", description: "Marks this helmet prefab as HMD-capable for global VehicleHelmetPrefabs.conf policy (full vehicle HMD when not using prefab list, or in addition to list matching).")]
class HMD_HelmetCapabilityComponentClass : ScriptComponentClass
{
}

//------------------------------------------------------------------------------------------------
//! Tag component: add to helmet prefabs that grant HMD HUD in vehicles (see Configs/HMD/VehicleHelmetPrefabs.conf).
class HMD_HelmetCapabilityComponent : ScriptComponent
{
}
