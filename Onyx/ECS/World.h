#pragma once

#include <map>
#include <set>
#include <memory>
#include <vector>
#include <algorithm>

#include "Entity.h"

namespace onyx::ecs
{

// forward declaration from "Onyx/ECS/Query.h"
struct IQuery;

// forward declaration from "Onyx/ECS/Scene.h"
struct Scene;

struct World
{
	template< typename ... Components >
	EntityID AddEntity( const Components& ... components )
	{
		const EntityID entity = m_nextEntityID++;
		AddComponents( entity, components ... );
		return entity;
	}

	void RemoveEntity( EntityID entity, bool and_children = false );

	void ResetEntities();

	template< typename Component >
	Component& AddComponent( EntityID entity, const Component& component )
	{
		return GetComponentTable< Component >().AddComponent( entity, component );
	}

	template< typename ... Components >
	void AddComponents( EntityID entity, const Components& ... components )
	{
		( AddComponent( entity, components ), ... );
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

	struct IComponentTable
	{
		virtual ~IComponentTable() = default;

		virtual void RemoveComponent( EntityID entity ) = 0;

		struct IIterator
		{
			virtual ~IIterator() = default;

			virtual EntityID ID() const = 0;
			virtual EntityID NextID() const = 0;
			virtual operator bool() const = 0;
			virtual void CopyToWorld( World& world, EntityID entity ) const = 0;

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
		Component& AddComponent( EntityID entity, const Component& component )
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

			return iter->second;
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
			if ( iter != m_components.end() && iter->first == entity )
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

			void CopyToWorld( World& world, EntityID entity ) const override
			{
				world.AddComponent( entity, Component() );
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

	struct EntityIterator
	{
		EntityID m_currentEntity = ~0;
		std::map< size_t, std::unique_ptr< IComponentTable::IIterator > > m_iterators;

		EntityIterator( const World& world, const std::set< size_t >* relevant_components );

		explicit operator bool() const;

		EntityIterator& operator ++();

		EntityID ID() const { return m_currentEntity; }

		EntityID CopyToWorld( World& world ) const;

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

	EntityIterator Iter( const std::set< size_t >* relevant_components = nullptr ) const
	{ return EntityIterator( *this, relevant_components ); }

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
		return const_cast<World*>( this )->GetOptionalComponentTableInternal< Component >();
	}
};

}
