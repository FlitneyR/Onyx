#pragma once

#include "World.h"
#include "Query.h"
#include "System.h"

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

	// add a subset of systems which have to run one after another
	template< typename ... Funcs >
	void AddSubset( Funcs* ... callbacks )
	{
		auto subset = std::make_unique< Subset >( m_querySet );
		( subset->AddSystem( callbacks ), ... );
		m_systems.push_back( std::move( subset ) );
	}

	void Run( Components& ... components )
	{
		IContext context( components ... );

		#pragma omp parallel for
		for ( i32 system_index = 0; system_index < m_systems.size(); ++system_index )
		{
			m_systems[ system_index ]->Run( context );
		}
	}

private:
	struct Subset : ISystem< IContext >
	{
		Subset( QuerySet& query_set )
			: m_querySet( query_set )
		{}

		void Run( IContext& context ) const override
		{
			for ( auto& system : m_systems )
				system->Run( context );
		}

		template< typename Func >
		void AddSystem( Func* callback )
		{
			m_systems.push_back( std::make_unique< System< IContext, Func > >( m_querySet, callback ) );
		}

		QuerySet& m_querySet;
		std::vector< std::unique_ptr< ISystem< IContext > > > m_systems;
	};

	QuerySet& m_querySet;
	std::vector< std::unique_ptr< ISystem< IContext > > > m_systems;
};

}
