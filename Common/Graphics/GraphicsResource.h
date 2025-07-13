#pragma once
#include "DeleteQueue.h"

namespace onyx
{

struct IGraphicsResource
{
	~IGraphicsResource()
	{ m_deleteQueue.Execute(); }

	DeleteQueue m_deleteQueue;
};

struct SharedResourceDeleter : DeleteQueue::IDeleter
{
	std::shared_ptr< IGraphicsResource > m_resource;

	SharedResourceDeleter( std::shared_ptr< IGraphicsResource > resource ) : m_resource( resource ) {}

	void Execute() { m_resource.reset(); }
};

}
