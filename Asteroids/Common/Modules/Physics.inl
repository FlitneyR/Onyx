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

	system_set.AddDependency( UpdatePhysicsBodies::System, UpdateCollisions::System );
	system_set.AddDependency( UpdateCollisions::System, UpdateDamageOnCollision::System );
	system_set.AddDependency( UpdatePhysicsBodies::System, onyx::Core::UpdateTransform2DLocales::System );
}

}
