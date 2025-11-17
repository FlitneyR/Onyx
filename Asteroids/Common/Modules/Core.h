#pragma once

#include "Onyx/ECS/Query.h"
#include "Onyx/ECS/Modules/Core.h"
#include "Onyx/ECS/Modules/Graphics2D.h"
#include "Onyx/ECS/SystemContexts.h"

#include "Onyx/Random.h"

namespace asteroids::Core
{

void RegisterReflectors( onyx::ecs::ComponentReflectorTable& table );

struct Camera
{
	f32 minFov = 1'000.f;
	f32 maxFov = 2'000.f;
	f32 margin = 3.f;
	f32 moveSpeed = 0.5f;
	f32 zoomSpeed = 0.5f;
	f32 fov = 0.5f * ( minFov + maxFov );
};

struct CameraFocus
{};

struct Health
{
	f32 max = 0.f;
	f32 amount = 0.f;
};

struct HealthForAnimation
{};

struct OnDeath
{
	std::shared_ptr< onyx::ecs::Scene > spawnScene;
};

struct OffScreenSpawner
{
	std::shared_ptr< onyx::ecs::Scene > prefab { nullptr };
	glm::vec2 linearVelocityRange { 0.f };
	glm::vec2 angularVelocityRange { 0.f };
	glm::vec2 spawnPeriodRange { 0.f };
	f32 timeToNextSpawn = -1.f;
	u32 spawnCount = 0;
};

struct Lifetime
{
	f32 duration;
	f32 remaining;
	bool active;
};

struct Team
{
	enum Enum
	{
		None = 0,
		Player,
		Enemy,
		Count,
	};

	Enum team;

	static bool AreFriends( const Team* lhs, const Team* rhs )
	{ return lhs && rhs && lhs->team && lhs->team == rhs->team; }
};

namespace UpdateCamera
{
using CameraFocii = onyx::ecs::Query<
	onyx::ecs::Read< onyx::Core::Transform2D >,
	onyx::ecs::Read< CameraFocus >
>;

using Cameras = onyx::ecs::Query<
	onyx::ecs::Write< onyx::Core::Transform2D >,
	onyx::ecs::Write< Camera >
>;

using Context = onyx::ecs::Context< const onyx::Tick, onyx::Camera2D >;

void System( Context ctx, const CameraFocii& focii, const Cameras& cameras );
}

namespace UpdateLifetimes
{
using Context = onyx::ecs::Context<
	const onyx::ecs::World,
	const onyx::Tick,
	onyx::AssetManager,
	onyx::ecs::CommandBuffer
>;

using Entities = onyx::ecs::Query<
	onyx::ecs::Write< Lifetime >
>;

void System( Context ctx, const Entities& entities );
}

namespace UpdateOffScreenSpawners
{
using Context = onyx::ecs::Context<
	const onyx::Tick,
	onyx::ecs::CommandBuffer,
	const onyx::Camera2D,
	const onyx::RNG
>;

using Spawners = onyx::ecs::Query<
	onyx::ecs::Write< OffScreenSpawner >
>;

void System( Context ctx, const Spawners& entities );
}

namespace UpdateHealthSprites
{
using Context = onyx::ecs::Context<>;

using Entities = onyx::ecs::Query<
	onyx::ecs::Read< Health >,
	onyx::ecs::Read< HealthForAnimation >,
	onyx::ecs::Write< onyx::Graphics2D::SpriteAnimator >
>;

void System( Context ctx, const Entities& entities );
}

struct DamageParams
{
	DamageParams() = default;
	DamageParams( const DamageParams& other ) = default;

	DamageParams& SetSource( onyx::ecs::EntityID source ) { this->source = source; return *this; }
	DamageParams& SetTarget( onyx::ecs::EntityID target ) { this->target = target; return *this; }
	DamageParams& SetAmount( f32 amount ) { this->amount = amount; return *this; }

	onyx::ecs::EntityID source = onyx::ecs::NoEntity;
	onyx::ecs::EntityID target = onyx::ecs::NoEntity;
	f32 amount = 0.f;
};

void HandleEntityDeath( const onyx::ecs::World& world, onyx::ecs::CommandBuffer& cmd, onyx::AssetManager& asset_manager, onyx::ecs::EntityID entity );
void DamageEntity( const onyx::ecs::World& world, onyx::ecs::CommandBuffer& cmd, onyx::AssetManager& asset_manager, const DamageParams& params );

}
