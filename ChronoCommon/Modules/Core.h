#pragma once

#include "Common/ECS/Query.h"

#include "Common/ECS/Modules/Core.h"
#include "Common/ECS/Modules/Graphics2D.h"
#include "Common/ECS/SystemContexts.h"

namespace chrono::Core
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
	f32 amount;
};

struct OnDeath
{
	std::shared_ptr< onyx::ecs::Scene > spawnScene;
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

using UpdateCamera_CameraFocusQuery = onyx::ecs::Query<
	onyx::ecs::Read< onyx::Core::Transform2D >,
	onyx::ecs::Read< CameraFocus >
>;

using UpdateCamera_CameraQuery = onyx::ecs::Query<
	onyx::ecs::Write< onyx::Core::Transform2D >,
	onyx::ecs::Write< Camera >
>;

void UpdateCamera( onyx::ecs::Context< const onyx::Tick, onyx::Camera2D > ctx, const UpdateCamera_CameraFocusQuery& focii, const UpdateCamera_CameraQuery& cameras );

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
