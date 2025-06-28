#pragma once
#include "Common/ECS/Query.h"
#include "Common/ECS/SystemContexts.h"
#include "Common/ECS/Components/Transform.h"
#include "Common/ECS/Components/Sprite.h"

#include "ChronoCommon/Components/Player.h"
#include "ChronoCommon/Components/Physics.h"

namespace chrono
{

using PlayerQuery = onyx::ecs::Query<
	onyx::ecs::Read< PlayerController >,
	onyx::ecs::Read< onyx::Transform2D >,
	onyx::ecs::Write< PhysicsBody >
>;

using PlayerEngineQuery = onyx::ecs::Query<
	onyx::ecs::Write< onyx::SpriteAnimator >
>;

void UpdatePlayers( onyx::ecs::Context< const onyx::Tick > ctx, const PlayerQuery& players, const PlayerEngineQuery& engines );

}
