#pragma once
#include "DeleteQueue.h"
#include "Common/Window.h"

#include <vector>

namespace onyx
{

struct IFrameContext;

struct IGraphicsContext
{
	virtual ~IGraphicsContext() {}

	virtual std::unique_ptr< IWindowContext > CreateWindowContext( IWindow& window ) = 0;

	virtual IFrameContext* BeginFrame( IWindow& window ) = 0;
	virtual void EndFrame( IFrameContext& frame_context ) = 0;

	DeleteQueue m_shutdownDeleteQueue;
};

}
