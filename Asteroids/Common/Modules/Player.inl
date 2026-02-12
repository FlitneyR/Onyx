#pragma once
#include "Player.h"
#include "Physics.h"

namespace asteroids::Player
{

template< typename SystemSet >
void RegisterGameplaySystems( SystemSet& system_set )
{
	system_set.AddSystem( UpdatePlayers::System );
	system_set.AddDependency( UpdatePlayers::System, asteroids::Physics::UpdatePhysicsBodies::System );
}

}
