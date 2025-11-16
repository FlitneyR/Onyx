#pragma once

#include "Onyx/ECS/Entity.h"
#include "Onyx/ECS/Query.h"
#include "Onyx/ECS/Modules/Core.h"
#include "Onyx/ECS/SystemContexts.h"

#include "Asteroids/Common/Modules/Core.h"

#include <vector>

namespace asteroids::Physics
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

namespace UpdateCollisions
{
using Context = onyx::ecs::Context< const onyx::Tick >;

using Entities = onyx::ecs::Query<
	onyx::ecs::Read< onyx::Core::Transform2D >,
	onyx::ecs::Write< Collider >
>;

void System( Context ctx, const Entities& colliders );
}

namespace UpdatePhysicsBodies
{
using Context = onyx::ecs::Context< const onyx::Tick >;

using Entities = onyx::ecs::Query<
	onyx::ecs::Write< PhysicsBody >,
	onyx::ecs::Write< onyx::Core::Transform2D >
>;

void System( Context ctx, const Entities& entities );
}

namespace UpdateDamageOnCollision
{
using Context = onyx::ecs::Context<
	const onyx::ecs::World,
	onyx::ecs::CommandBuffer,
	onyx::AssetManager
>;

using Entities = onyx::ecs::Query<
	onyx::ecs::Read< Collider >,
	onyx::ecs::Read< DamageOnCollision >,
	onyx::ecs::WriteOptional< asteroids::Core::Health >,
	onyx::ecs::ReadOptional< asteroids::Core::Team >
>;

void System( Context ctx, const Entities& damagers );
}

}
