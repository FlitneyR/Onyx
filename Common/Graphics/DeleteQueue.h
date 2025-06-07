#pragma once

#include <deque>

namespace onyx
{

struct DeleteQueue
{
	struct IDeleter { virtual void Execute() = 0; };

	template< typename Deleter, typename ... Args >
	void Add( Args ... args )
	{
		m_queue.push_back( std::make_unique< Deleter >( args ... ) );
	}

	void Execute()
	{
		while ( !m_queue.empty() )
		{
			m_queue.back()->Execute();
			m_queue.pop_back();
		}
	}

private:
	std::deque< std::unique_ptr< IDeleter > > m_queue;
};

}
