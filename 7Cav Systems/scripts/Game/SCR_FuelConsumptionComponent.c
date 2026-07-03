[BaseContainerProps()]
modded class SCR_FuelConsumptionComponent
{
    // Do not redeclare s_fGlobalFuelConsumptionScale here: the vanilla class already
    // declares that static (protected, default 8), so a modded redeclaration conflicts.
    static const float CAV_GLOBAL_FUEL_CONSUMPTION_SCALE = 4.0;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        if (Replication.IsServer())
            SetGlobalFuelConsumptionScale(CAV_GLOBAL_FUEL_CONSUMPTION_SCALE);
    }
}
