[ComponentEditorProps(category: "Belt Linking", description: "Enables belt linking - spawns action at given position. Allows another player to reload this weapon without animation.")]
class BL_BeltLinkWeaponComponentClass: ScriptComponentClass
{
}

class BL_BeltLinkWeaponComponent: ScriptComponent
{
	[Attribute("0 0 0", UIWidgets.EditBox, "Action position offset from weapon origin (local space)")]
	protected vector m_vActionPosition = "0 0 0";

	[Attribute("2", UIWidgets.EditBox, "Max distance from the belt-link interaction point (helper) to the linker, in meters")]
	protected float m_fLinkRange = 2.0;

	[Attribute("{7C4B4E4C13985C01}Prefabs/Weapons/Helpers/BL_LinkBeltActionHelper.et", UIWidgets.ResourceNamePicker, "Action helper prefab (Link Belt user action)")]
	protected ResourceName m_sActionHelperPrefab;

	protected ref array<IEntity> m_aActionHelpers = new array<IEntity>();

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// The helper prefab carries an RplComponent, so it must only be spawned by
		// the authority; clients receive it through replication. Spawning it on
		// every machine gave clients a second, local-only helper whose "Link belt"
		// action could never reach the server.
		if (!Replication.IsServer())
			return;

		SpawnActionHelper();
	}

	override void OnDelete(IEntity owner)
	{
		for (int i = 0; i < m_aActionHelpers.Count(); i++)
		{
			IEntity helper = m_aActionHelpers.Get(i);
			if (helper)
				SCR_EntityHelper.DeleteEntityAndChildren(helper);
		}
		m_aActionHelpers.Clear();
		super.OnDelete(owner);
	}

	protected void SpawnActionHelper()
	{
		IEntity weaponEntity = GetOwner();
		vector worldMat[4];
		weaponEntity.GetWorldTransform(worldMat);
		vector worldPos = worldMat[3] + worldMat[0] * m_vActionPosition[0] + worldMat[1] * m_vActionPosition[1] + worldMat[2] * m_vActionPosition[2];

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[0] = worldMat[0];
		params.Transform[1] = worldMat[1];
		params.Transform[2] = worldMat[2];
		params.Transform[3] = worldPos;

		vector localMat[4];
		localMat[0] = vector.Right;
		localMat[1] = vector.Up;
		localMat[2] = vector.Forward;
		localMat[3] = m_vActionPosition;

		Resource prefab = Resource.Load(m_sActionHelperPrefab);
		if (!prefab || !prefab.IsValid())
			return;

		IEntity spawned = GetGame().SpawnEntityPrefab(prefab, weaponEntity.GetWorld(), params);
		if (!spawned)
			return;

		weaponEntity.AddChild(spawned, -1);
		spawned.SetLocalTransform(localMat);
		m_aActionHelpers.Insert(spawned);
	}

	float GetLinkRange()
	{
		return m_fLinkRange;
	}

	vector GetActionPosition()
	{
		return m_vActionPosition;
	}
}
