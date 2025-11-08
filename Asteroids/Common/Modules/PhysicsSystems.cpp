#include "Physics.h"

#include "tracy/Tracy.hpp"

namespace asteroids::Physics
{

void UpdateCollisions( onyx::ecs::Context< const onyx::Tick > ctx, const ColliderQuery& colliders )
{
	ZoneScoped;

	for ( auto& entity : colliders )
		entity.Get< Collider& >().collisions.clear();

	for ( u32 first_idx = 0; first_idx + 1 < colliders.Count(); ++first_idx )
	{
		auto [first_id, first_transform, first_collider] = colliders[ first_idx ].Break();
		const glm::vec2 first_world_pos = first_transform.GetWorldPosition();

		for ( u32 second_idx = first_idx + 1; second_idx < colliders.Count(); ++second_idx )
		{
			auto [second_id, second_transform, second_collider] = colliders[ second_idx ].Break();
			const glm::vec2 second_world_pos = second_transform.GetWorldPosition();

			const float sqr_dist = glm::distance2( first_world_pos, second_world_pos );
			const float sum_of_square_radii = first_collider.radius * first_collider.radius + second_collider.radius * second_collider.radius;

			if ( sqr_dist < sum_of_square_radii )
			{
				const float mix_factor = first_collider.radius / ( first_collider.radius + second_collider.radius );
				const glm::vec2 collision_point = glm::mix( first_world_pos, second_world_pos, mix_factor );

				CollisionEvent first_event;
				CollisionEvent second_event;

				first_event.otherEntity = second_id;
				second_event.otherEntity = first_id;
				first_event.collisionPoint = second_event.collisionPoint = collision_point;

				first_collider.collisions.push_back( first_event );
				second_collider.collisions.push_back( second_event );
			}
		}
	}
}

void UpdatePhysicsBodies( onyx::ecs::Context< const onyx::Tick > ctx, const PhysicsBodyQuery& bodies )
{
	ZoneScoped;

	const onyx::Tick& tick = ctx.Get< const onyx::Tick >();

	for ( auto& entity : bodies )
	{
		auto [id, body, transform] = entity.Break();

		#ifndef NDEBUG
		WEAK_ASSERT_ONCE( transform.GetLocale() == glm::mat3( 1.f ), "Players should be in world space, and shouldn't have a locale" );
		#endif

		glm::vec2 position = transform.GetLocalPosition();
		f32 rotation = transform.GetLocalRotation();

		position += body.linearVelocity * tick.deltaTime;
		rotation += body.angularVelocity * tick.deltaTime;

		body.linearVelocity *= glm::clamp( 1.0f - body.linearFriction * tick.deltaTime, 0.f, 1.f );
		body.angularVelocity *= glm::clamp( 1.0f - body.angularFriction * tick.deltaTime, 0.f, 1.f );

		transform.SetLocalPosition( position );
		transform.SetLocalRotation( rotation );
	}
}

void UpdateDamageOnCollision( UpdateDamageOnCollision_Context ctx, const DamageOnCollisionEntities& damagers )
{
	ZoneScoped;

	auto [world, cmd, asset_manager] = ctx.Break();

	for ( const auto& damager : damagers )
	{
		auto [damager_id, damager_collider, damager_damage, damager_health, damager_team] = damager.Break();

		for ( const CollisionEvent& collision : damager_collider.collisions )
		{
			WEAK_ASSERT_ONCE( damager_id != collision.otherEntity );

			Core::DamageEntity( world, cmd, asset_manager, Core::DamageParams()
				.SetSource( damager_id )
				.SetTarget( collision.otherEntity )
				.SetAmount( damager_damage.otherDamage ) );

			Core::DamageEntity( world, cmd, asset_manager, Core::DamageParams()
				.SetSource( collision.otherEntity )
				.SetTarget( damager_id )
				.SetAmount( damager_damage.selfDamage ) );
		}
	}
}

}
