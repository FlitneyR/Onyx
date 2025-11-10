#include "World.h"

#include "Modules/Core.h"

namespace onyx::ecs
{

void World::ResetEntities()
{
	m_componentTables.clear();
	m_nextEntityID = 1;
}

void World::RemoveEntity( EntityID entity, bool and_children )
{
	ZoneScoped;

	for ( auto& [hash, table] : m_componentTables )
		table->RemoveComponent( entity );

	if ( and_children )
	{
		if ( ComponentTable< onyx::Core::AttachedTo >* attached_to_table = GetOptionalComponentTable< onyx::Core::AttachedTo >() )
		{
			std::vector< EntityID > children;

			for ( auto attached_to_iter = attached_to_table->Iter(); attached_to_iter; attached_to_iter.GoToNext() )
				if ( attached_to_iter.GetComponent()->localeEntity == entity )
					children.push_back( attached_to_iter.GetEntityID() );

			for ( EntityID child : children )
				RemoveEntity( child, and_children );
		}
	}
}

World::EntityIterator::EntityIterator( const World& world, const std::set< size_t >* relevant_components )
{
	for ( auto& [hash, table] : world.m_componentTables )
	{
		if ( relevant_components && !relevant_components->contains( hash ) )
			continue;

		IComponentTable::IIterator iter( *table );

		// INFO( "iter.m_index = {}, iter.GetEntityID() = {}", iter.m_index, iter.GetEntityID() );

		if ( const EntityID first_entity = iter.GetEntityID() )
			m_currentEntity = std::min< u32 >( m_currentEntity, first_entity );

		m_iterators.insert( { hash, iter } );
	}
}

World::EntityIterator& World::EntityIterator::operator ++()
{
	// find the lowest next entity ID
	EntityID lowest_next_entity_id = UINT32_MAX;
	for ( auto& [_, iter] : m_iterators )
	{
		if ( const EntityID curr = iter.GetEntityID(); curr > m_currentEntity )
			lowest_next_entity_id = std::min( lowest_next_entity_id, curr );
		else if ( const EntityID next = iter.GetNextEntityID() )
			lowest_next_entity_id = std::min( lowest_next_entity_id, next );
	}

	// progress to that entity ID
	m_currentEntity = lowest_next_entity_id;
	for ( auto& [_, iter] : m_iterators )
		while ( iter && iter.GetEntityID() < lowest_next_entity_id )
			++iter;

	return *this;
}

EntityID World::EntityIterator::CopyToWorld( World& world ) const
{
	const EntityID entity = world.AddEntity();

	for ( const auto& [type, comp] : m_iterators )
		if ( comp.GetEntityID() == GetEntityID() )
			comp.CopyToWorld( world, entity );

	return entity;
}

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

IComponentTable* World::GetComponentTableByHash( size_t component_type_hash )
{
	auto iter = m_componentTables.find( component_type_hash );
	return iter == m_componentTables.end() ? nullptr : iter->second.get();
}

}
