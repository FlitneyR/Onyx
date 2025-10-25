#pragma once
#include "DeleteQueue.h"
#include "Onyx/Window.h"
#include "FrameContext.h"

#include <vector>

#include "Onyx/Graphics/Texture.h"
#include "Onyx/Graphics/SpriteRenderer.h"

namespace onyx
{

struct IGraphicsContext
{
	virtual ~IGraphicsContext() {}

	virtual std::unique_ptr< IWindowContext > CreateWindowContext( IWindow& window ) = 0;

	virtual IFrameContext* BeginFrame( IWindow& window ) = 0;
	virtual void EndFrame( IFrameContext& frame_context ) = 0;

	virtual std::shared_ptr< IRenderTarget > CreateRenderTarget( const glm::uvec2& dimensions ) = 0;
	virtual std::shared_ptr< ITextureResource > CreateTextureResource( const TextureAsset& texture ) = 0;
	virtual std::shared_ptr< ISpriteRenderer > CreateSpriteRenderer() = 0;

	DeleteQueue m_shutdownDeleteQueue;
};

}
