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

			for ( auto attached_to_iter = attached_to_table->Iter(); attached_to_iter; ++attached_to_iter )
				if ( attached_to_iter.Component().localeEntity == entity )
					children.push_back( attached_to_iter.ID() );

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

		auto iter = table->GenericIter();

		if ( EntityID entity = iter->ID() )
			m_currentEntity = std::min< u32 >( m_currentEntity, entity );

		m_iterators.insert( { hash, std::move( iter ) } );
	}
}

World::EntityIterator::operator bool() const
{
	for ( auto& [_, iter] : m_iterators )
		if ( *iter )
			return true;

	return false;
}

World::EntityIterator& World::EntityIterator::operator ++()
{
	// find the next entity id for any component
	EntityID next_entity = ~0;
	for ( const auto& [hash, iterator] : m_iterators )
	{
		if ( const EntityID curr_id = iterator->ID(); curr_id > m_currentEntity )
			next_entity = std::min< u32 >( curr_id, next_entity );
		else if ( const EntityID next_id = iterator->NextID() )
			next_entity = std::min< u32 >( next_id, next_entity );
	}

	m_currentEntity = next_entity;

	// progress iterators for each component to at least that entity
	for ( auto& [hash, iterator] : m_iterators )
		while ( *iterator && iterator->ID() < m_currentEntity )
			iterator->Increment();

	return *this;
}

EntityID World::EntityIterator::CopyToWorld( World& world ) const
{
	const EntityID entity = world.AddEntity();

	for ( const auto& [type, comp] : m_iterators )
		if ( comp->ID() == ID() )
			comp->CopyToWorld( world, entity );

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

World::IComponentTable* World::GetComponentTableByHash( size_t component_type_hash )
{
	auto iter = m_componentTables.find( component_type_hash );
	return iter == m_componentTables.end() ? nullptr : iter->second.get();
}

}
