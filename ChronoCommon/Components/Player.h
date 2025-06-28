#pragma once

#include "Common/ECS/Entity.h"

#include "Common/Graphics/Texture.h"

namespace chrono
{

struct PlayerController
{
	f32 boostSpeed = 1.f;
	f32 turnSpeed = 1.f;

	std::shared_ptr< onyx::TextureAnimationAsset > idleEngineAnimation;
	std::shared_ptr< onyx::TextureAnimationAsset > boostEngineAnimation;
	onyx::ecs::EntityID engineSprite;
};

}
