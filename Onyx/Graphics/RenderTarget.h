#pragma once

#include "GraphicsResource.h"

namespace onyx
{

struct IFrameContext;

struct IRenderTarget : IGraphicsResource
{
	IRenderTarget( const glm::uvec2& size ) : m_size( size ) {}

	virtual void Clear( IFrameContext& frame_ctx, const glm::vec4& colour ) {}
	virtual void PrepareForRendering( IFrameContext& frame_ctx ) {}
	virtual void PrepareForCompositing( IFrameContext& frame_ctx ) {}
	virtual void PrepareForSampling( IFrameContext& frame_ctx ) {}

	virtual ImTextureID GetImTextureID() = 0;

	const glm::uvec2& GetSize() { return m_size; }

private:
	const glm::uvec2 m_size;
};

}
