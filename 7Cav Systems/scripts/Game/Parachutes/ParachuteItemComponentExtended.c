class ParachuteItemComponentExtendedClass : ParachuteItemComponentClass {}
class ParachuteItemComponentExtended : ParachuteItemComponent
{
	[Attribute("{B1C2D3E4F5A6B7C8}Prefabs/DeployedParachutes/Core/DeployedParachuteMK4Extended.et", UIWidgets.ResourceNamePicker, "Parachute Prefab (Extended)", "et")]
	protected ResourceName m_ExtendedParachutePrefab;

	// Reset to unused when pilot exits chute without landing (e.g. double jump during invincibility)
	void SetParachuteUnused_Server()
	{
		if (!Replication.IsServer())
			return;

		if (!m_bParachuteUsed)
			return;

		m_bParachuteUsed = false;
		Replication.BumpMe();
	}

	override ResourceName GetParachutePrefab()
	{
		if (m_ExtendedParachutePrefab != "")
			return m_ExtendedParachutePrefab;
		return super.GetParachutePrefab();
	}
}
