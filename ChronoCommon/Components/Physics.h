#pragma once

#include "Common/ECS/Entity.h"

#include <vector>

namespace chrono
{

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

}
