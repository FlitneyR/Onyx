#pragma once

#include "Core.h"

#include "Onyx/Graphics/Camera.h"
#include "Onyx/Graphics/SpriteRenderer.h"
#include "Onyx/Graphics/Texture.h"

#include "Onyx/ECS/Query.h"
#include "Onyx/ECS/System.h"
#include "Onyx/ECS/SystemContexts.h"

namespace onyx::Graphics2D
{

void RegisterReflectors( onyx::ecs::ComponentReflectorTable& table );

struct Sprite
{
private:
	std::shared_ptr< ITextureResource > texture;

public:
	std::shared_ptr< TextureAsset > textureAsset;
	glm::vec2 offset { 0.f, 0.f };
	glm::vec2 extent { 1.f, 1.f };
	u32 layer = 1;

	std::shared_ptr< ITextureResource > GetTextureResource()
	{
		if ( textureAsset && !texture )
			texture = textureAsset->GetGraphicsResource();

		return texture;
	}

	void SetTexture( std::shared_ptr< TextureAsset > texture_asset )
	{
		if ( texture_asset == textureAsset )
			return;

		textureAsset = texture_asset;
		texture.reset();
	}

	inline const char* GetAssetPath() const { return !textureAsset ? "" : textureAsset->m_path.c_str(); }

	COMPONENT_REFLECTOR_FRIEND( Sprite );
};

struct SpriteAnimator
{
	std::shared_ptr< TextureAnimationAsset > animation = nullptr;
	f32 currentFrame = 0;
	f32 playRate = 1.f;
	bool loop = true;
};

struct ParallaxBackground
{
	glm::vec2 scrollRate;
	glm::vec2 scale;
};

namespace CollectSprites
{
using Context = ecs::Context< SpriteRenderData >;

using Entities = ecs::Query<
	ecs::Read< Core::Transform2D >,
	ecs::Write< Sprite >
>;

void System( Context ctx, const Entities& entities );
}

namespace UpdateAnimatedSprites
{
using Context = ecs::Context< const Tick >;

using Entities = ecs::Query<
	ecs::Write< SpriteAnimator >,
	ecs::Write< Sprite >
>;

void System( Context ctx, const Entities& entities );
}

namespace UpdateParallaxBackgroundLayers
{
using Context = ecs::Context< const Camera2D >;

using Entities = ecs::Query<
	ecs::Write< Sprite >,
	ecs::Write< Core::Transform2D >,
	ecs::ReadOptional< SpriteAnimator >,
	ecs::Read< ParallaxBackground >
>;

void System( Context ctx, const Entities& entities );
}

}

