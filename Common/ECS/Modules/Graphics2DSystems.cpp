#include "Graphics2D.h"

namespace onyx::Graphics2D
{

void CollectSprites( ecs::Context< SpriteRenderData > ctx, const CollectSpritesQuery& entities )
{
	auto [render_data] = ctx.Break();

	for ( auto& entity : entities )
	{
		const auto& [ id, transform, sprite ] = entity.Break();

		auto texture = sprite.GetTextureResource();

		if ( !texture.get() )
			continue;

		const auto& [ iter, is_new ] = render_data.textureIndices.insert( { texture.get(), render_data.textures.size() } );
		const auto& [ _, texture_index ] = *iter;

		if ( is_new )
			render_data.textures.push_back( texture );

		render_data.spriteInstances[ sprite.layer ].push_back( {
			transform.GetMatrix(),
			sprite.offset,
			sprite.extent,
			texture_index
		} );
	}
}

void UpdateAnimatedSprites( ecs::Context< const Tick > ctx, const UpdateAnimatedSpritesQuery& animated_sprites )
{
	auto [tick] = ctx.Break();

	for ( auto& entity : animated_sprites )
	{
		auto [id, animator, sprite] = entity.Break();

		TextureAnimationAsset* animation = animator.animation.get();
		if ( !animation )
			continue;

		animator.currentFrame = std::fmodf( animator.currentFrame + tick.deltaTime * animator.playRate, (f32)animation->m_frames.size() );

		TextureAnimationAsset::Frame& frame = animation->m_frames[ animator.currentFrame ];
		if ( !frame.texture )
			continue;

		sprite.SetTexture( frame.texture );
		sprite.offset = frame.offset / frame.denom;
		sprite.extent = frame.extent / frame.denom;
	}
}

void UpdateParallaxBackgroundLayers( ecs::Context< const Camera2D > ctx, const UpdateParallaxBackgroundLayersQuery& background_layers )
{
	auto [camera] = ctx.Break();

	for ( auto& layer : background_layers )
	{
		auto [id, sprite, transform, animator, background] = layer.Break();

		#ifndef NDEBUG
		LOG_ASSERT_ONCE( transform.GetLocale() == glm::mat3( 1.f ), "A parallax background layer is attached to something, panic!" );
		#endif

		// align self with the camera
		transform.SetLocalPosition( camera.position );

		// cover the camera view area
		transform.SetLocalScale( glm::vec2( 2.f * camera.fov * glm::max( camera.aspectRatio.x, camera.aspectRatio.y ) ) );

		// reset the offset and extent if an animator hasn't
		if ( !animator )
		{
			sprite.offset = {};
			sprite.extent = { 1.f, 1.f };
		}

		// scale the frame selected by the sprite animator
		const glm::vec2 scale = transform.GetLocalScale() * background.scale * sprite.extent;
		const glm::vec2 offset = transform.GetLocalPosition() * background.scrollRate * background.scale / glm::normalize( glm::vec2( sprite.extent.y, sprite.extent.x ) );

		sprite.extent = scale;
		sprite.offset += offset - sprite.extent / 2.f;
	}
}

}
