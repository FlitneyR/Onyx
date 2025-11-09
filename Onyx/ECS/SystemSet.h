#pragma once

#include "World.h"
#include "Query.h"
#include "System.h"
#include "Onyx/Multithreading.h"

#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace onyx::ecs
{

template< typename ... Components >
struct SystemSet
{
	using IContext = Context< Components ... >;

	SystemSet( QuerySet& query_set ) : m_querySet( query_set ) {}

	// add a system to run in parallel with everything else
	template< typename Func >
	void AddSystem( Func* callback )
	{
		m_systems.insert( { (u64)callback, std::make_unique< System< IContext, Func > >( m_querySet, callback ) } );
	}

	void AddDependency( void* first, void* second )
	{
		auto iter = m_dependencies.find( (u64)first );
		if ( iter != m_dependencies.end() )
		{
			iter->second.insert( (u64)second );
			return;
		}

		m_dependencies.insert( (u64)first, { (u64)second });
	}

private:
	QuerySet& m_querySet;
	std::unordered_map< u64, std::unordered_set< u64 > > m_dependencies;
	std::unordered_map< u64, std::unique_ptr< ISystem< IContext > > > m_systems;

	struct RunSystemJob : IJob
	{
		RunSystemJob( ISystem< IContext >* system, IContext* context )
			: m_system( *system )
			, m_context( *context )
		{}

		void Run() override { m_system.Run( m_context ); }

	private:
		ISystem< IContext >& m_system;
		IContext& m_context;
	};

public:
	void Run( Components& ... components )
	{
		ZoneScoped;

		IContext context( components ... );

		WorkerPool& worker_pool = onyx::LowLevel::GetWorkerPool();
		JobQueue& job_queue = worker_pool.GetJobQueue();

		job_queue.Reserve( m_systems.size() );
		for ( auto& [_, system] : m_systems )
			job_queue.AddJob< RunSystemJob >( system.get(), &context );

		worker_pool.Begin();
		worker_pool.Wait();
	}
};

}
