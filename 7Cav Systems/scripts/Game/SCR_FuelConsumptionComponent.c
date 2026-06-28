[BaseContainerProps()]
modded class SCR_FuelConsumptionComponent
{
    static float s_fGlobalFuelConsumptionScale = 4.0;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        if (Replication.IsServer())
            SetGlobalFuelConsumptionScale(s_fGlobalFuelConsumptionScale);
    }
}
