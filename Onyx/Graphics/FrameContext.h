#pragma once
#include "DeleteQueue.h"
#include "GraphicsResource.h"

#include <set>

// this was the original way to track which graphics resources had been used in a frame
// it has been more thoroughly tested, but also showed up in the profiler as being quite slow
#define USE_DELETE_QUEUE_TO_TRACK_FRAME_RESOURCES 0

namespace onyx
{

struct IRenderTarget;

struct IFrameContext
{
	void RegisterUsedResource( std::shared_ptr< IGraphicsResource > resource )
	{
		#if USE_DELETE_QUEUE_TO_TRACK_FRAME_RESOURCES
		m_deleteQueue.Add< SharedResourceDeleter >(resource);
		#else
		m_usedResources.insert( resource );
		#endif
	}
	
	virtual void BlitRenderTarget( std::shared_ptr< IRenderTarget >& render_target, glm::uvec2 position, glm::uvec2 size ) = 0;

	virtual glm::uvec2 GetSize() const = 0;

	// call when we know the frame has finished rendering, let go of the frame's resources
	void OnFinishFrame()
	{
		ZoneScoped;

		#if USE_DELETE_QUEUE_TO_TRACK_FRAME_RESOURCES
		m_deleteQueue.Execute();
		#else
		m_usedResources.clear();
		#endif
	}

protected:
	#if USE_DELETE_QUEUE_TO_TRACK_FRAME_RESOURCES
	DeleteQueue m_deleteQueue;
	#else
	std::set< std::shared_ptr< IGraphicsResource > > m_usedResources;
	#endif
};

}
