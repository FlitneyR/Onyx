#pragma once
#include "Physics.h"
#include "Onyx/ECS/Modules/Core.h"

namespace asteroids::Physics
{

template< typename SystemSet >
void RegisterGameplaySystems( SystemSet& system_set )
{
	system_set.AddSystem( UpdatePhysicsBodies::System );
	system_set.AddSystem( UpdateCollisions::System );
	system_set.AddSystem( UpdateDamageOnCollision::System );

	system_set.AddDependency( (void*)UpdatePhysicsBodies::System, (void*)UpdateCollisions::System );
	system_set.AddDependency( (void*)UpdateCollisions::System, (void*)UpdateDamageOnCollision::System );
	system_set.AddDependency( (void*)UpdatePhysicsBodies::System, (void*)onyx::Core::UpdateTransform2DLocales::System );
}

}
