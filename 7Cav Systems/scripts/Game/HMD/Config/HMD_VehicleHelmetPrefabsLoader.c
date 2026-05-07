//------------------------------------------------------------------------------------------------
//! Loads HMD_VehicleHelmetPrefabsConfig from a .conf resource and caches the prefab list.
//! Pattern (searchable in BI docs / catalog scripts): Resource.Load -> GetResource().ToBaseContainer()
//! -> BaseContainerTools.CreateInstanceFromContainer(BaseContainer) -> Cast to your config class.
class HMD_VehicleHelmetPrefabsLoader
{
	protected static const ResourceName DEFAULT_CONF = "{6900000800000001}Configs/HMD/VehicleHelmetPrefabs.conf";

	protected static ref map<string, ref array<ResourceName>> s_CacheByPath;

	//! Parsed from DEFAULT_CONF only (Laser HUDMarkerSystem m_bEnforceHmdHelmetInVehicles parity).
	protected static bool s_GlobalEnforceHmdHelmetInVehicles;

	//------------------------------------------------------------------------------------------------
	static bool HasGlobalHelmetPrefabList()
	{
		array<ResourceName> list = GetHelmetPrefabs(ResourceName.Empty);
		return list && list.Count() > 0;
	}

	//------------------------------------------------------------------------------------------------
	static bool GetGlobalEnforceHmdHelmetInVehicles()
	{
		GetHelmetPrefabs(ResourceName.Empty);
		return s_GlobalEnforceHmdHelmetInVehicles;
	}

	//------------------------------------------------------------------------------------------------
	static void ApplyGlobalEnforceFromConfig(HMD_VehicleHelmetPrefabsConfig cfg, ResourceName resolvedPath)
	{
		if (resolvedPath != DEFAULT_CONF)
			return;
		s_GlobalEnforceHmdHelmetInVehicles = false;
		if (cfg)
			s_GlobalEnforceHmdHelmetInVehicles = cfg.GetEnforceHmdHelmetInVehicles();
	}

	//------------------------------------------------------------------------------------------------
	static notnull array<ResourceName> GetHelmetPrefabs(ResourceName configOverride)
	{
		if (!s_CacheByPath)
			s_CacheByPath = new map<string, ref array<ResourceName>>();
		ResourceName path = configOverride;
		if (path.IsEmpty())
			path = DEFAULT_CONF;
		string key = path;
		if (s_CacheByPath.Contains(key))
		{
			array<ResourceName> cached = s_CacheByPath.Get(key);
			if (cached)
				return cached;
		}
		ref array<ResourceName> built = new array<ResourceName>();
		Resource res = Resource.Load(path);
		if (!res || !res.IsValid())
		{
			ApplyGlobalEnforceFromConfig(null, path);
			s_CacheByPath.Set(key, built);
			return built;
		}
		BaseContainer bc = res.GetResource().ToBaseContainer();
		if (!bc)
		{
			ApplyGlobalEnforceFromConfig(null, path);
			s_CacheByPath.Set(key, built);
			return built;
		}
		Managed inst = BaseContainerTools.CreateInstanceFromContainer(bc);
		HMD_VehicleHelmetPrefabsConfig cfg = HMD_VehicleHelmetPrefabsConfig.Cast(inst);
		ApplyGlobalEnforceFromConfig(cfg, path);
		if (!cfg || !cfg.m_aHelmetPrefabs)
		{
			s_CacheByPath.Set(key, built);
			return built;
		}
		foreach (ResourceName rn : cfg.m_aHelmetPrefabs)
		{
			if (!rn.IsEmpty())
				built.Insert(rn);
		}
		s_CacheByPath.Set(key, built);
		return built;
	}

	//------------------------------------------------------------------------------------------------
	static bool EntityPrefabMatchesHelmetList(IEntity ent, ResourceName configOverride)
	{
		if (!ent)
			return false;
		EntityPrefabData pfd = ent.GetPrefabData();
		if (!pfd)
			return false;
		ResourceName itemRn = pfd.GetPrefabName();
		if (itemRn.IsEmpty())
			return false;
		array<ResourceName> list = GetHelmetPrefabs(configOverride);
		if (!list || list.Count() < 1)
			return false;
		foreach (ResourceName allowed : list)
		{
			if (allowed == itemRn)
				return true;
			if (SCR_BaseContainerTools.IsKindOf(itemRn, allowed))
				return true;
		}
		return false;
	}
}
