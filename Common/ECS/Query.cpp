#include "Query.h"
#include <mutex>
#include <set>

namespace onyx::ecs
{

void World::QueryManager::UpdateNeedsRerun( World& world )
{
	std::erase_if( m_queries, []( const std::pair< size_t, std::weak_ptr< IQuery > >& pair )
	{
		return pair.second.expired();
	} );

	std::set< size_t > changed_components;

	for ( auto& [hash, table] : world.m_componentTables )
	{
		if ( table->m_hasChanged )
		{
			changed_components.insert( hash );
			table->m_hasChanged = false;
		}
	}

	for ( const size_t& component_type_hash : changed_components )
		for ( auto& [hash, query] : m_queries )
			query.lock()->OnComponentAddedOrRemoved( component_type_hash );
}

void QuerySet::Update()
{
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
				query->ClearResults();
				query->CollectComponentTypes( relevant_components );
				queries_to_run.push_back( query );
			}
		}
	}

	// if any queries need to rerun, run them
	if ( !queries_to_run.empty() )
		for ( auto iter = m_world.Iter( &relevant_components ); iter; ++iter )
			for ( auto query : queries_to_run )
				query->Consider( iter );

	// mark that these no longer need to rerun
	for ( auto query : queries_to_run )
		query->ResetNeedsRerun();
}

}
