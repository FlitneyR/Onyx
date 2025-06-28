#pragma once
#include "DeleteQueue.h"
#include "GraphicsResource.h"

namespace onyx
{

struct IRenderTarget;

struct IFrameContext
{
	DeleteQueue m_deleteQueue;

	void RegisterUsedResource( std::shared_ptr< IGraphicsResource > resource )
	{ m_deleteQueue.Add< SharedResourceDeleter >( resource ); }
	
	virtual void BlitRenderTarget( std::shared_ptr< IRenderTarget >& render_target, glm::uvec2 position, glm::uvec2 size ) = 0;

	virtual glm::uvec2 GetSize() const = 0;
};

}
