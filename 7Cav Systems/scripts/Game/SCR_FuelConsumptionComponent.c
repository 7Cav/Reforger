[BaseContainerProps()]
modded class SCR_FuelConsumptionComponent
{
    // Change this value to whatever multiplier you want
    // 1.0  = realistic fuel consumption
    // 2.0  = somewhat realistic
    // 4.0  = balanced
    // 8.0  = vanilla default (very high consumption)
    static float s_fGlobalFuelConsumptionScale = 4.0;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        // Apply the global fuel consumption scale (server only)
        if (Replication.IsServer())
            SetGlobalFuelConsumptionScale(s_fGlobalFuelConsumptionScale);
    }
}
