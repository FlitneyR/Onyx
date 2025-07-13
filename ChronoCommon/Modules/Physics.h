#pragma once

#include "Common/ECS/Entity.h"
#include "Common/ECS/Query.h"
#include "Common/ECS/Modules/Core.h"
#include "Common/ECS/SystemContexts.h"

#include "ChronoCommon/Modules/Core.h"

#include <vector>

namespace chrono::Physics
{

void RegisterReflectors( onyx::ecs::ComponentReflectorTable& table );

struct PhysicsBody
{
	f32 linearFriction = {};
	f32 angularFriction = {};
	glm::vec2 linearVelocity = {};
	f32 angularVelocity = {};
};

struct CollisionEvent
{
	onyx::ecs::EntityID otherEntity = {};
	glm::vec2 collisionPoint = {};
};

struct Collider
{
	f32 radius;
	std::vector< CollisionEvent > collisions;
};

struct DamageOnCollision
{
	f32 selfDamage;
	f32 otherDamage;
};

using ColliderQuery = onyx::ecs::Query<
	onyx::ecs::Read< onyx::Core::Transform2D >,
	onyx::ecs::Write< Collider >
>;

void UpdateCollisions( onyx::ecs::Context< const onyx::Tick > ctx, const ColliderQuery& colliders );

using PhysicsBodyQuery = onyx::ecs::Query<
	onyx::ecs::Write< PhysicsBody >,
	onyx::ecs::Write< onyx::Core::Transform2D >
>;

void UpdatePhysicsBodies( onyx::ecs::Context< const onyx::Tick > ctx, const PhysicsBodyQuery& bodies );

using DamageOnCollisionEntities = onyx::ecs::Query<
	onyx::ecs::Read< Collider >,
	onyx::ecs::Read< DamageOnCollision >,
	onyx::ecs::WriteOptional< chrono::Core::Health >,
	onyx::ecs::ReadOptional< chrono::Core::Team >
>;

using UpdateDamageOnCollision_Context = onyx::ecs::Context<
	const onyx::ecs::World,
	onyx::ecs::CommandBuffer,
	onyx::AssetManager
>;

void UpdateDamageOnCollision( UpdateDamageOnCollision_Context ctx, const DamageOnCollisionEntities& damagers );

}
