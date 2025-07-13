#include "Core.h"

namespace chrono::Core
{

void UpdateCamera( onyx::ecs::Context< const onyx::Tick, onyx::Camera2D > ctx, const UpdateCamera_CameraFocusQuery& focii, const UpdateCamera_CameraQuery& cameras )
{
	auto [tick, ctx_camera] = ctx.Break();

	glm::vec2 focus_mean = {};
	f32 focus_count = 0.f;

	for ( auto& entity : focii )
	{
		auto [id, transform, focus] = entity.Break();

		focus_mean += transform.LocalToWorld( transform.GetLocalPosition() );
		focus_count += 1.f;
	}

	focus_mean /= focus_count;

	f32 max_distance = 0.f;

	for ( auto& entity : focii )
	{
		auto [id, transform, focus] = entity.Break();

		max_distance = std::max< f32 >( max_distance, glm::length( focus_mean - transform.LocalToWorld( transform.GetLocalPosition() ) ) );
	}

	for ( auto& entity : cameras )
	{
		auto [id, transform, camera] = entity.Break();

		const f32 move_lerp_factor = std::clamp( camera.moveSpeed * tick.deltaTime, 0.f, 1.f );
		const f32 zoom_lerp_factor = std::clamp( camera.zoomSpeed * tick.deltaTime, 0.f, 1.f );

		const f32 dist = std::max< f32 >( max_distance, glm::length( focus_mean - transform.LocalToWorld( transform.GetLocalPosition() ) ) );

		// assuming that cameras have no locale
		const f32 target_fov = std::clamp< f32 >( dist * camera.margin, camera.minFov, camera.maxFov );

		camera.fov = target_fov * zoom_lerp_factor + camera.fov * ( 1.f - zoom_lerp_factor );
		transform.SetLocalPosition( focus_mean * move_lerp_factor + transform.GetLocalPosition() * ( 1.f - move_lerp_factor ) );

		if ( !ctx_camera.entity )
			ctx_camera.entity = id;

		if ( ctx_camera.entity == id )
		{
			ctx_camera.position = transform.GetLocalPosition();
			ctx_camera.rotation = transform.GetLocalRotation();
			ctx_camera.fov = camera.fov;
		}
	}
}

void HandleEntityDeath( const onyx::ecs::World& world, onyx::ecs::CommandBuffer& cmd, onyx::AssetManager& asset_manager, onyx::ecs::EntityID entity )
{
	if ( const OnDeath* const target_on_death = world.GetComponent< OnDeath >( entity ) )
	{
		if ( target_on_death->spawnScene && target_on_death->spawnScene->GetLoadingState() == onyx::LoadingState::Loaded )
		{
			glm::mat3 transform( 1.f );

			if ( const onyx::Core::Transform2D* target_transform = world.GetComponent< onyx::Core::Transform2D >( entity ) )
				transform = target_transform->GetMatrix();

			cmd.CopySceneToWorld( target_on_death->spawnScene,
				[ transform ]
				( onyx::ecs::World& world, const onyx::ecs::IDMap& entities )
				{
					onyx::Core::PostCopyUpdateRootTransforms2D( world, entities, transform );
				}
			);
		}
	}

	cmd.RemoveEntity( entity );
}

void DamageEntity( const onyx::ecs::World& world, onyx::ecs::CommandBuffer& cmd, onyx::AssetManager& asset_manager, const DamageParams& params )
{
	if ( params.amount == 0.f )
		return;

	if ( Team::AreFriends( world.GetComponent< Team >( params.source ), world.GetComponent< Team >( params.target ) ) )
		return;

	if ( Health* const target_health = world.GetComponent< Health >( params.target ) )
		if ( ( target_health->amount -= params.amount ) > 0.f )
			return;

	HandleEntityDeath( world, cmd, asset_manager, params.target );
}

}
