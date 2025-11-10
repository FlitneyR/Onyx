#pragma once

#include "World.h"
#include "Query.h"
#include "System.h"
#include "Onyx/Multithreading.h"

#include <vector>
#include <memory>

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
		m_systems.push_back( std::make_unique< System< IContext, Func > >( m_querySet, callback ) );
	}

	void AddDependency( void* first, void* second )
	{
		m_dependencies.push_back( { (u64)first, (u64)second } );
	}

private:
	QuerySet& m_querySet;
	std::vector< std::tuple< u64, u64 > > m_dependencies;
	std::vector< std::unique_ptr< ISystem< IContext > > > m_systems;

	struct RunSystemJob : IJob
	{
		RunSystemJob( ISystem< IContext >& system, IContext& context )
			: m_system( system )
			, m_context( context )
		{}

		void Run() override { m_system.Run( m_context ); }

	private:
		ISystem< IContext >& m_system;
		IContext m_context;
	};

public:
	void Run( Components& ... components )
	{
		ZoneScoped;

		IContext context( components ... );

		WorkerPool& worker_pool = onyx::LowLevel::GetWorkerPool();
		JobQueue& job_queue = worker_pool.GetJobQueue();

		job_queue.Reserve( m_systems.size() );

		{
			ZoneScopedN( "Adding System Jobs to queue" );

			for ( auto& system : m_systems )
				job_queue.AddJob< RunSystemJob >( system->GetID(), *system, context );
		}

		{
			ZoneScopedN( "Adding dependencies to queue" );

			for ( auto& [first, second] : m_dependencies )
				if ( IJob* first_job = job_queue.GetJob( first ) )
					if ( IJob* second_job = job_queue.GetJob( second ) )
						second_job->AddDependency( first_job );
		}

		worker_pool.Begin();
	}
};

}
