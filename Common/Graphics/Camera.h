#pragma once

#include "Common/ECS/Entity.h"

namespace onyx
{

struct Camera2D
{
	glm::vec2 position {};
	f32 rotation {};
	glm::vec2 aspectRatio { 1.f, 1.f };
	f32 fov {};

	ecs::EntityID entity = ecs::NoEntity;

	glm::mat3 GetMatrix() const;
};

}
