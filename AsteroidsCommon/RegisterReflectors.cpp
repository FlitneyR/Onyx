#include "RegisterReflectors.h"

#include "Common/ECS/Modules/Core.h"
#include "Common/ECS/Modules/Graphics2D.h"

#include "AsteroidsCommon/Modules/Core.h"
#include "AsteroidsCommon/Modules/Physics.h"
#include "AsteroidsCommon/Modules/Player.h"

void RegisterReflectors()
{
	onyx::Core::RegisterReflectors( onyx::ecs::ComponentReflectorTable::s_singleton );
	onyx::Graphics2D::RegisterReflectors( onyx::ecs::ComponentReflectorTable::s_singleton );
	asteroids::Core::RegisterReflectors( onyx::ecs::ComponentReflectorTable::s_singleton );
	asteroids::Physics::RegisterReflectors( onyx::ecs::ComponentReflectorTable::s_singleton );
	asteroids::Player::RegisterReflectors( onyx::ecs::ComponentReflectorTable::s_singleton );
}