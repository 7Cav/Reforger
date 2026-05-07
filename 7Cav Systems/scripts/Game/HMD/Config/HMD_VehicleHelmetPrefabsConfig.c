//------------------------------------------------------------------------------------------------
//! Root type for Configs/HMD/VehicleHelmetPrefabs.conf (helmet item prefabs for vehicle HMD gate).
//! configRoot: true matches Bohemia "Create a Config Class" so this type is a valid .conf root in Workbench.
[BaseContainerProps(configRoot: true)]
class HMD_VehicleHelmetPrefabsConfig
{
	[Attribute(desc: "Helmet / attachment prefab ResourceNames that count as HMD (exact match or IsKindOf vs equipped attachment prefabs under the character).")]
	ref array<ResourceName> m_aHelmetPrefabs;

	[Attribute("0", UIWidgets.CheckBox, "When set, require HMD helmet (capability tag or prefab list match) in vehicles even if the helmet prefab list above is empty.", category: "HMD")]
	protected bool m_bEnforceHmdHelmetInVehicles;

	bool GetEnforceHmdHelmetInVehicles()
	{
		return m_bEnforceHmdHelmetInVehicles;
	}
}
