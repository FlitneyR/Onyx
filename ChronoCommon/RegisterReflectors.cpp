#include "RegisterReflectors.h"

#include "Common/ECS/Modules/Core.h"
#include "Common/ECS/Modules/Graphics2D.h"

#include "ChronoCommon/Modules/Core.h"
#include "ChronoCommon/Modules/Physics.h"
#include "ChronoCommon/Modules/Player.h"

void RegisterReflectors()
{
	onyx::Core::RegisterReflectors( onyx::ecs::ComponentReflectorTable::s_singleton );
	onyx::Graphics2D::RegisterReflectors( onyx::ecs::ComponentReflectorTable::s_singleton );
	chrono::Core::RegisterReflectors( onyx::ecs::ComponentReflectorTable::s_singleton );
	chrono::Physics::RegisterReflectors( onyx::ecs::ComponentReflectorTable::s_singleton );
	chrono::Player::RegisterReflectors( onyx::ecs::ComponentReflectorTable::s_singleton );
}