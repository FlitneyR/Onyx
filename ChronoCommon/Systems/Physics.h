#pragma once
#include "Common/ECS/Query.h"
#include "Common/ECS/SystemContexts.h"
#include "Common/ECS/Components/Transform.h"

#include "ChronoCommon/Components/Physics.h"

namespace chrono
{

using ColliderQuery = onyx::ecs::Query<
	onyx::ecs::Read< onyx::Transform2D >,
	onyx::ecs::Write< chrono::Collider >
>;

void UpdateCollisions( onyx::ecs::Context<> ctx, const ColliderQuery& colliders );

using PhysicsBodyQuery = onyx::ecs::Query<
	onyx::ecs::Write< PhysicsBody >,
	onyx::ecs::Write< onyx::Transform2D >
>;

void UpdatePhysicsBodies( onyx::ecs::Context< const onyx::Tick > ctx, const PhysicsBodyQuery& bodies );

}
