#pragma once

#include <map>
#include <set>
#include <memory>
#include <vector>
#include <algorithm>

#include "Entity.h"

namespace onyx::ecs
{

// forward declaration from "Common/ECS/Query.h"
struct IQuery;

struct World
{
	template< typename ... Components >
	EntityID AddEntity( Components ... components )
	{
		const EntityID entity = m_nextEntityID++;

		// kindly ignore this horrible incantation
		( [ & ]() { AddComponent( entity, components ); return true; }( ) && ... );

		return entity;
	}

	void RemoveEntity( EntityID entity )
	{
		for ( auto& [hash, table] : m_componentTables )
			table->RemoveComponent( entity );
	}

	void ResetEntities()
	{
		m_componentTables.clear();
		m_nextEntityID = 1;
	}

	template< typename Component >
	void AddComponent( EntityID entity, const Component& component )
	{
		GetComponentTable< Component >().AddComponent( entity, component );
	}

	template< typename Component >
	void RemoveComponent( EntityID entity )
	{
		GetComponentTable< Component >().RemoveComponent( entity );
	}

	template< typename Component >
	Component* GetComponent( EntityID entity )
	{
		return GetComponentTable< Component >().GetComponent( entity );
	}

	struct IComponentTable
	{
		virtual ~IComponentTable() {}

		virtual void RemoveComponent( EntityID entity ) = 0;

		struct IIterator
		{
			virtual EntityID ID() const = 0;
			virtual EntityID NextID() const = 0;
			virtual operator bool() const = 0;

			void Increment() { ++m_currentPairIndex; }

		protected:
			u32 m_currentPairIndex = 0;
		};

		virtual std::unique_ptr< IIterator > GenericIter() = 0;

		bool m_hasChanged = false;
	};

	template< typename Component >
	struct ComponentTable : IComponentTable
	{
		void AddComponent( EntityID entity, const Component& component )
		{
			auto iter = std::lower_bound( m_components.begin(), m_components.end(), entity, PairEntityIDComparator );
			if ( iter == m_components.end() || iter->first != entity )
			{
				iter = m_components.insert( iter, { entity, component } );
				m_hasChanged = true;
			}
			else
			{
				iter->second = component;
			}
		}

		Component* GetComponent( EntityID entity )
		{
			auto iter = std::lower_bound( m_components.begin(), m_components.end(), entity, PairEntityIDComparator );
			if ( iter == m_components.end() || iter->first != entity )
				return nullptr;

			return &iter->second;
		}

		void RemoveComponent( EntityID entity ) override
		{
			auto iter = std::lower_bound( m_components.begin(), m_components.end(), entity, PairEntityIDComparator );
			if ( iter != m_components.end() )
			{
				m_components.erase( iter );
				m_hasChanged = true;
			}
		}

		struct Iterator : IIterator
		{
			Iterator( ComponentTable& table ) : m_table( table ) {}

			operator bool() const override { return ID() != NoEntity; }
			Iterator& operator ++() { Increment(); return *this; }

			EntityID ID() const override
			{
				if ( m_currentPairIndex >= m_table.m_components.size() )
					return NoEntity;

				return m_table.m_components[ m_currentPairIndex ].first;
			}

			EntityID NextID() const override
			{
				const u32 next_index = m_currentPairIndex + 1;
				if ( next_index >= m_table.m_components.size() )
					return NoEntity;

				return m_table.m_components[ next_index ].first;
			}

			Component& Component() const { return m_table.m_components[ m_currentPairIndex ].second; }

		private:

			ComponentTable& m_table;
		};

		Iterator Iter() { return Iterator( *this ); }
		std::unique_ptr< IIterator > GenericIter() override { return std::make_unique< Iterator >( *this ); }

		using Pair = std::pair< EntityID, Component >;

	private:
		std::vector< Pair > m_components;

		static bool PairEntityIDComparator( const Pair& lhs, const EntityID& entity ) { return lhs.first < entity; }
	};

	struct BaseEntityIterator
	{
		EntityID ID() const { return m_currentEntity; }

		template< typename Component >
		Component* Get() const
		{
			auto _iter = m_iterators.find( typeid( Component ).hash_code() );
			if ( _iter == m_iterators.end() )
				return nullptr;

			auto& iter = *static_cast<ComponentTable< Component >::Iterator*>( _iter->second.get() );
			if ( !iter || iter.ID() != ID() )
				return nullptr;

			return &iter.Component();
		}

	protected:
		u32 m_currentEntity = ~0;
		std::map< size_t, std::unique_ptr< IComponentTable::IIterator > > m_iterators;
	};

	struct EntityIterator : BaseEntityIterator
	{
		EntityIterator( World& world, const std::set< size_t >* relevant_components )
		{
			for ( auto& [hash, table] : world.m_componentTables )
			{
				if ( relevant_components && !relevant_components->contains( hash ) )
					continue;

				auto iter = table->GenericIter();

				if ( EntityID entity = iter->ID() )
					m_currentEntity = std::min( m_currentEntity, entity );

				m_iterators.insert( { hash, std::move( iter ) } );
			}
		}

		operator bool() const
		{
			for ( auto& [_, iter] : m_iterators )
				if ( *iter )
					return true;

			return false;
		}

		EntityIterator& operator ++()
		{
			// find the next entity id for any component
			//EntityID next_entity = ~0;

			m_currentEntity = ~0;
			for ( const auto& [hash, iterator] : m_iterators )
				if ( const EntityID entity = iterator->NextID() )
					m_currentEntity = std::min( entity, m_currentEntity );

			// progress iterators for each component to at least that entity
			for ( auto& [hash, iterator] : m_iterators )
				while ( *iterator && iterator->ID() < m_currentEntity )
					iterator->Increment();

			return *this;
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

	EntityIterator Iter( const std::set< size_t >* relevant_components = nullptr )
	{
		return EntityIterator( *this, relevant_components );
	}

private:
	EntityID m_nextEntityID = 1;
	std::map< size_t, std::unique_ptr< IComponentTable > > m_componentTables;

	IComponentTable* GetComponentTableByHash( size_t component_type_hash )
	{
		auto iter = m_componentTables.find( component_type_hash );
		return iter == m_componentTables.end() ? nullptr : iter->second.get();
	}

public:

	template< typename Component >
	ComponentTable< Component >& GetComponentTable()
	{
		const size_t component_type_hash = typeid( Component ).hash_code();

		auto iter = m_componentTables.find( component_type_hash );
		if ( iter == m_componentTables.end() )
			iter = m_componentTables.insert( { component_type_hash, std::make_unique< ComponentTable< Component > >() } ).first;

		return *static_cast<ComponentTable< Component >*>( iter->second.get() );
	}
};

}
