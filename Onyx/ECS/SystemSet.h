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
	std::vector< std::unique_ptr< const ISystem< IContext > > > m_systems;

	struct RunSystemJob : IJob
	{
		RunSystemJob( const ISystem< IContext >& system, const IContext& context )
			: m_system( system )
			, m_context( context )
		{}

		void Run() override { m_system.Run( m_context ); }

	private:
		const ISystem< IContext >& m_system;
		const IContext m_context;
	};

public:
	void Run( Components& ... components )
	{
		ZoneScoped;

		IContext context( components ... );

		WorkerPool& worker_pool = onyx::LowLevel::GetWorkerPool();
		JobQueue& job_queue = worker_pool.GetJobQueue();

		job_queue.Reserve( (u32)m_systems.size() );

		{
			ZoneScopedN( "Adding System Jobs to queue" );

			for ( auto& system : m_systems )
				job_queue.AddJob< RunSystemJob >( system->GetID(), *system, context );
		}

		{
			ZoneScopedN( "Adding dependencies to queue" );

			for ( auto& [first, second] : m_dependencies )
				if ( IJob* const first_job = WEAK_ASSERT( job_queue.GetJob( first ) ) )
					if ( IJob* const second_job = WEAK_ASSERT( job_queue.GetJob( second ) ) )
						second_job->AddDependency( first_job );
		}

		worker_pool.Begin();
	}
};

}
