#pragma once

#include "Common/Graphics/Texture.h"

namespace onyx
{

struct Sprite
{
	std::shared_ptr< ITextureResource > m_texture;
	glm::vec2 offset { 0.f, 0.f };
	glm::vec2 extent { 1.f, 1.f };
	u32 layer = 1;
};

struct SpriteAnimator
{
	std::shared_ptr< TextureAnimationAsset > m_animation = nullptr;
	f32 m_currentFrame = 0;
	f32 m_playRate = 1.f;
};

struct ParallaxBackground
{
	glm::vec2 scrollRate;
	glm::vec2 scale;
};

}
