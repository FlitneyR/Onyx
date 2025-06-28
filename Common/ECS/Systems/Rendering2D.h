#pragma once

#include "Common/ECS/Components/Sprite.h"
#include "Common/ECS/Components/Transform.h"
#include "Common/ECS/Query.h"
#include "Common/ECS/System.h"
#include "Common/ECS/SystemContexts.h"

#include "Common/Graphics/Camera.h"
#include "Common/Graphics/SpriteRenderer.h"

namespace onyx
{

using CollectSpritesQuery = ecs::Query<
	ecs::Read< Transform2D >,
	ecs::Read< Sprite >
>;

void CollectSprites( ecs::Context< SpriteRenderData > ctx, const CollectSpritesQuery& entities );

using UpdateAnimatedSpritesQuery = ecs::Query<
	ecs::Write< SpriteAnimator >,
	ecs::Write< Sprite >
>;

void UpdateAnimatedSprites( ecs::Context< const Tick > ctx, const UpdateAnimatedSpritesQuery& animated_sprites );

using UpdateParallaxBackgroundLayersQuery = ecs::Query<
	ecs::Write< Sprite >,
	ecs::Write< Transform2D >,
	ecs::ReadOptional< SpriteAnimator >,
	ecs::Read< ParallaxBackground >
>;

void UpdateParallaxBackgroundLayers( ecs::Context< const Camera2D > ctx, const UpdateParallaxBackgroundLayersQuery& background_layers );

}
