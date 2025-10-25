#include "RegisterReflectors.h"

#include "Onyx/ECS/Modules/Core.h"
#include "Onyx/ECS/Modules/Graphics2D.h"

#include "Asteroids/Common/Modules/Core.h"
#include "Asteroids/Common/Modules/Physics.h"
#include "Asteroids/Common/Modules/Player.h"

void RegisterReflectors()
{
	onyx::Core::RegisterReflectors( onyx::ecs::ComponentReflectorTable::s_singleton );
	onyx::Graphics2D::RegisterReflectors( onyx::ecs::ComponentReflectorTable::s_singleton );
	asteroids::Core::RegisterReflectors( onyx::ecs::ComponentReflectorTable::s_singleton );
	asteroids::Physics::RegisterReflectors( onyx::ecs::ComponentReflectorTable::s_singleton );
	asteroids::Player::RegisterReflectors( onyx::ecs::ComponentReflectorTable::s_singleton );
}