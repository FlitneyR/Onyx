#pragma once

#include "World.h"
#include "Query.h"
#include "System.h"

#include <vector>
#include <memory>

namespace onyx::ecs
{

template< typename Global >
struct SystemSet
{
	SystemSet( QuerySet& query_set ) : m_querySet( query_set ) {}

	template< typename Func >
	void AddSystem( Func* callback )
	{
		m_systems.push_back( std::make_unique< System< Global, Func > >( m_querySet, callback ) );
	}

	void Run( Global& global )
	{
		#pragma omp parallel for
		for ( i32 system_index = 0; system_index < m_systems.size(); ++system_index )
		{
			m_systems[ system_index ]->Run( global );
		}
	}

private:
	QuerySet& m_querySet;
	std::vector< std::unique_ptr< ISystem< Global > > > m_systems;
};

}
