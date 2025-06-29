#include "Physics.h"

namespace chrono
{

void UpdateCollisions( onyx::ecs::Context<> ctx, const ColliderQuery& colliders )
{
	for ( auto& entity : colliders )
	{
		auto [id, transform, collider] = entity.Break();
		collider.collisions.clear();
	}

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
	auto [tick] = ctx.Break();

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

}
