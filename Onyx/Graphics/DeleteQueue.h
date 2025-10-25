#pragma once

#include <deque>
#include <memory>

#define DEBUG_HANGING_DELETERS 0

namespace onyx
{

struct DeleteQueue
{
	struct IDeleter { virtual void Execute() = 0; virtual void LogDebugInfo() { ERROR( "Unexecuted, unknown deleter" ); } };

	#if DEBUG_HANGING_DELETERS
	static std::vector< std::weak_ptr< IDeleter > > s_allDeleters;
	#endif

	static void CheckForHangingDeleters()
	{
		#if DEBUG_HANGING_DELETERS
		for ( auto& _deleter : s_allDeleters )
		{
			if ( auto deleter = _deleter.lock() )
				deleter->LogDebugInfo();
		}
		#endif
	}

	~DeleteQueue() { Execute(); }

	template< typename Deleter, typename ... Args >
	void Add( Args ... args )
	{
		#if DEBUG_HANGING_DELETERS
		auto deleter = std::make_shared< Deleter >( args ... );
		s_allDeleters.push_back( deleter );
		m_queue.push_back( deleter );
		#else
		m_queue.push_back( std::make_unique< Deleter >( args ... ) );
		#endif
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
	#if DEBUG_HANGING_DELETERS
	std::deque< std::shared_ptr< IDeleter > > m_queue;
	#else
	std::deque< std::unique_ptr< IDeleter > > m_queue;
	#endif
};

}
