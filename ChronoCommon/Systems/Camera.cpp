#include "Camera.h"

namespace chrono
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

}
