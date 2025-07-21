#pragma once

#include "Common/ECS/Entity.h"

#include "Common/ECS/Modules/Core.h"
#include "Common/ECS/Modules/Graphics2D.h"
#include "Common/ECS/Scene.h"
#include "Common/Graphics/Texture.h"

#include "Physics.h"

namespace chrono::Player
{

void RegisterReflectors( onyx::ecs::ComponentReflectorTable& table );

struct PlayerController
{
	f32 boostSpeed = 1.f;
	f32 turnSpeed = 1.f;
	f32 timeBetweenShots = 0.1f;

	std::shared_ptr< onyx::ecs::Scene > bulletPrefab;
	std::shared_ptr< onyx::TextureAnimationAsset > idleEngineAnimation;
	std::shared_ptr< onyx::TextureAnimationAsset > boostEngineAnimation;
	onyx::ecs::EntityID engineEffectSprite;

	f32 lastShotTime = -FLT_MAX;
};

struct Projectile
{
	f32 initialSpeed = 1.f;
};

using UpdatePlayers_Context = onyx::ecs::Context<
	const onyx::Tick,
	onyx::AssetManager,
	onyx::ecs::CommandBuffer
>;

using PlayerQuery = onyx::ecs::Query<
	onyx::ecs::Read< PlayerController >,
	onyx::ecs::Read< onyx::Core::Transform2D >,
	onyx::ecs::Write< chrono::Physics::PhysicsBody >
>;

using PlayerEngineQuery = onyx::ecs::Query<
	onyx::ecs::Write< onyx::Graphics2D::SpriteAnimator >
>;

void UpdatePlayers( UpdatePlayers_Context ctx, const PlayerQuery& players, const PlayerEngineQuery& engines );

}
