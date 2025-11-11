#include "Query.h"
#include <mutex>
#include <set>

#include "tracy/Tracy.hpp"

namespace onyx::ecs
{

void QuerySet::Update()
{
	ZoneScoped;

	// remove any queries that are no longer in use
	std::erase_if( m_queries, []( std::pair< size_t, std::weak_ptr< IQuery > > pair ) {
		return pair.second.expired();
	} );

	std::vector< std::shared_ptr< IQuery > > queries_to_run;
	std::set< size_t > relevant_components;
	queries_to_run.reserve( m_queries.size() );

	// find the queries that need to rerun
	for ( auto [_hash, _query] : m_queries )
	{
		if ( auto query = _query.lock() )
		{
			if ( query->NeedsRerun() )
			{
				query->ResetNeedsRerun();
				query->CollectComponentTypes( relevant_components );
				queries_to_run.push_back( query );
			}
		}
	}

	// if any queries need to rerun, run them
	if ( !queries_to_run.empty() )
	{
		ZoneScopedN( "Iterate Entities" );

		for ( World::EntityIterator iter = m_world.Iter( &relevant_components, true ); iter; ++iter )
		{
			for ( auto query : queries_to_run )
				query->Consider( iter );
		}
	}
}

}
