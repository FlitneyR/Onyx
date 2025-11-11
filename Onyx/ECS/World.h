#pragma once

#include <map>
#include <set>
#include <memory>
#include <vector>
#include <algorithm>

#include "Entity.h"
#include "ComponentTable.h"

#include "tracy/Tracy.hpp"

namespace onyx::ecs
{

// forward declaration from "Onyx/ECS/Query.h"
struct IQuery;

// forward declaration from "Onyx/ECS/Scene.h"
struct Scene;

struct World
{
	template< typename ... Components >
	EntityID AddEntity( Components&& ... components )
	{
		const EntityID entity = m_nextEntityID++;
		AddComponents( entity, std::move( components ) ... );
		return entity;
	}

	void RemoveEntity( EntityID entity, bool and_children = false );

	void ResetEntities();

	template< typename Component >
	Component& AddComponent( EntityID entity, Component&& component )
	{
		return GetComponentTable< Component >().AddComponent( entity, std::move( component ) );
	}

	template< typename ... Components >
	void AddComponents( EntityID entity, Components&& ... components )
	{
		( AddComponent( entity, std::move( components ) ), ... );
	}

	template< typename Component >
	void RemoveComponent( EntityID entity )
	{
		GetComponentTable< Component >().RemoveComponent( entity );
	}

	template< typename Component >
	Component* GetComponent( EntityID entity ) const
	{
		return GetComponentTable< Component >().GetComponent( entity );
	}

	struct EntityIterator
	{
		std::map< size_t, IComponentTable::IIterator > m_iterators;
		EntityID m_currentEntity = UINT32_MAX;
		bool m_dirtyOnly = false;

		EntityIterator() = default;
		EntityIterator( const World& world, const std::set< size_t >* relevant_components = nullptr, bool dirty_only = false );

		operator bool() const { return (u32)GetEntityID() < UINT32_MAX; }
		EntityIterator& operator ++();

		EntityID GetEntityID() const { return m_currentEntity; }

		void ResetDirtyFlags() { for ( auto& [_, iter] : m_iterators ) iter.RemoveDirtyFlag(); }

		EntityID CopyToWorld( World& world ) const;

		template< typename Component >
		Component* Get() const
		{
			auto iter = m_iterators.find( typeid( Component ).hash_code() );
			if ( iter == m_iterators.end() || iter->second.GetEntityID() != GetEntityID() )
				return nullptr;

			return reinterpret_cast< const typename ComponentTable< Component >::Iterator* >( &iter->second )->GetComponent();
		}
	};

	struct QueryManager
	{
		template< typename Query >
		std::shared_ptr< Query > Get()
		{
			const size_t query_type_hash = typeid( Query ).hash_code();

			auto iter = m_queries.find( query_type_hash );
			if ( iter == m_queries.end() )
				iter = m_queries.insert( { query_type_hash, {} } ).first;

			std::shared_ptr< IQuery > query = iter->second.lock();
			if ( !query )
				iter->second = query = std::make_shared< Query >();

			return std::static_pointer_cast<Query>( query );
		}

		void UpdateNeedsRerun( World& world );

	private:
		std::map< size_t, std::weak_ptr< IQuery > > m_queries;
	};

	QueryManager m_queryManager;

	EntityIterator Iter( const std::set< size_t >* relevant_components = nullptr, bool dirty_only = false ) const
	{ return EntityIterator( *this, relevant_components, dirty_only ); }

	void CleanUpPages();

private:
	std::map< size_t, std::unique_ptr< IComponentTable > > m_componentTables;
	EntityID m_nextEntityID { 1 };

	friend struct Scene;

	IComponentTable* GetComponentTableByHash( size_t component_type_hash );

	template< typename Component >
	ComponentTable< Component >& GetComponentTableInternal()
	{
		const size_t component_type_hash = typeid( Component ).hash_code();

		auto iter = m_componentTables.find( component_type_hash );
		if ( iter == m_componentTables.end() )
			iter = m_componentTables.insert( { component_type_hash, std::make_unique< ComponentTable< Component > >() } ).first;

		return *static_cast<ComponentTable< Component >*>( iter->second.get() );
	}

	template< typename Component >
	ComponentTable< Component >* GetOptionalComponentTableInternal()
	{
		return static_cast<ComponentTable< Component >*>( GetComponentTableByHash( typeid( Component ).hash_code() ) );
	}

public:

	template< typename Component >
	ComponentTable< Component >& GetComponentTable() const
	{
		return const_cast< World* >( this )->GetComponentTableInternal< Component >();
	}
	
	template< typename Component >
	ComponentTable< Component >* GetOptionalComponentTable() const
	{
		return const_cast< World* >( this )->GetOptionalComponentTableInternal< Component >();
	}
};

template< typename Component >
void ComponentTable< Component >::CopyComponentToWorld( World& world, IPage& page, u8 index, EntityID entity_id ) const
{
	if ( Component* component = reinterpret_cast< Page& >( page ).GetComponent( index ) )
		world.AddComponent( entity_id, Component( *component ) );
}

}
