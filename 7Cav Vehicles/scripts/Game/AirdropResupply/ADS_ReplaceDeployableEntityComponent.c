[EntityEditorProps(category: "GameScripted/DeployableItems", description: "RHS replace deployable with configurable post-ground delay")]
class ADS_ReplaceDeployableEntityComponentClass : RHS_ReplaceDeployableEntityComponentClass
{
}

class ADS_ReplaceDeployableEntityComponent : RHS_ReplaceDeployableEntityComponent
{
	[Attribute("2000", desc: "Delay after ground contact before Deploy runs [ms]", params: "0 inf")]
	protected int m_iDeployDelayMs;

	override void EOnPostFixedFrame(IEntity owner, float timeSlice)
	{
		Physics physics = owner.GetPhysics();
		if (!physics)
			return;

		vector ownerPosition = owner.GetOrigin();
		vector velocity = physics.GetVelocity();

		if (m_WeatherManager)
		{
			vector force;
			force = {0, m_WeatherManager.GetWindDirection(), 0};
			force = force.AnglesToVector();
			force = {force[1], 0, force[2]};
			force *= m_WeatherManager.GetWindSpeed();
			m_vWindForce = force * (-m_fWindInfluenceMultiplier);
		}

		RandomGenerator generator = new RandomGenerator();
		float seed = m_RplComponent.Id();
		if (m_WeatherManager)
			seed = seed + m_WeatherManager.GetEngineTime();
		generator.SetSeed(seed);
		float randomInfluence;
		if (m_fRandomWindInfluence != 0)
			randomInfluence = generator.RandFloatXY(-m_fRandomWindInfluence, m_fRandomWindInfluence);

		velocity[0] = velocity[0] + m_vWindForce[0] * timeSlice + randomInfluence;
		velocity[2] = velocity[2] + m_vWindForce[2] * timeSlice + randomInfluence;

		physics.SetVelocity(velocity);
		if (ownerPosition[1] + 1 <= owner.GetWorld().GetSurfaceY(ownerPosition[0], ownerPosition[2]))
		{
			ClearEventMask(owner, EntityEvent.POSTFIXEDFRAME);
			GetGame().GetCallqueue().CallLater(DeployWrapper, m_iDeployDelayMs, false, null, false);
			return;
		}

		if (Math.AbsFloat(velocity[1]) < 0.5)
		{
			ClearEventMask(owner, EntityEvent.POSTFIXEDFRAME);
			GetGame().GetCallqueue().CallLater(DeployWrapper, m_iDeployDelayMs, false, null, false);
		}
	}
}
