#pragma once

#include "Texture.h"

#include <array>
#include <vector>

namespace onyx
{

struct SpriteRenderData
{
	std::unordered_map< ITextureResource*, u32 > textureIndices;
	std::vector< std::shared_ptr< ITextureResource > > textures;

	static constexpr u32 s_maxLayers = 256;
	
	struct SpriteInstance
	{
		// 3x4 matrix to match the layout of a 3x3 matrix in glsl
		alignas( 16 )
		glm::mat3x4 transform;
		glm::vec2 offset;
		glm::vec2 extent;
		u32 textureIndex;
	};

	std::array< std::vector< SpriteInstance >, s_maxLayers > spriteInstances;

	// 3x4 matrix to match the layout of a 3x3 matrix in glsl
	glm::mat3x4 cameraMatrix;
};

struct ISpriteRenderer 
{
	virtual void Render( IFrameContext& frame, std::shared_ptr< IRenderTarget >& render_target, const SpriteRenderData& data ) = 0;
};

}
