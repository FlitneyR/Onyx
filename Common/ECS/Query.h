#pragma once

#include "World.h"

#include <set>

namespace onyx::ecs
{

struct IQuery
{
	virtual void ClearResults() = 0;
	virtual void Consider( const World::EntityIterator& entity ) = 0;
	virtual void OnComponentAddedOrRemoved( size_t component_type_hash ) = 0;
	virtual void CollectComponentTypes( std::set< size_t >& component_set ) = 0;

	bool NeedsRerun() const { return m_needsRerun; }
	void ResetNeedsRerun() { m_needsRerun = false; }

protected:
	bool m_needsRerun = true;
};

template< typename T >
struct Read
{
	using Type = T;
	using Ptr = T*;
	using Arg = const T&;
	static constexpr bool c_isRequired = true;

	static Arg Cast( Ptr ptr ) { return *ptr; }
};

template< typename T >
struct Write
{
	using Type = T;
	using Ptr = T*;
	using Arg = T&;
	static constexpr bool c_isRequired = true;

	static Arg Cast( Ptr ptr ) { return *ptr; }
};

template< typename T >
struct ReadOptional
{
	using Type = T;
	using Ptr = T*;
	using Arg = const T*;
	static constexpr bool c_isRequired = false;

	static Arg Cast( Ptr ptr ) { return ptr; }
};

template< typename T >
struct WriteOptional
{
	using Type = T;
	using Ptr = T*;
	using Arg = T*;
	static constexpr bool c_isRequired = false;

	static Arg Cast( Ptr ptr ) { return ptr; }
};

template< typename ... Components >
struct Context
{
	Context( Components& ... components )
		: m_components( components ... )
	{}

	template< typename OtherContext >
	Context( OtherContext& other_context )
		: m_components( std::get< std::remove_const_t< Components >& >( other_context.m_components ) ... )
	{}

	std::tuple< Components& ... > Break() { return m_components; }

private:
	template< typename ... OtherComponents >
	friend struct Context;

	std::tuple< Components& ... > m_components;
};

template< typename ... Components >
struct Query : IQuery
{
	struct Result
	{
		using Query = Query;

		Result( const World::EntityIterator& entity )
			: m_entity( entity.ID() )
			, m_componentPtrs( entity.Get< typename Components::Type >() ... )
		{}

		EntityID ID() const { return m_entity; }

		std::tuple< const EntityID&, typename Components::Arg ... > Break() const
		{
			return {
				m_entity,
				Components::Cast( std::get< typename Components::Ptr >( m_componentPtrs ) ) ...
			};
		}

	private:
		friend Query;
		bool IsComplete() const { return ( ( !Components::c_isRequired || std::get< typename Components::Ptr >( m_componentPtrs ) ) && ... ); }
		operator bool() const { return IsComplete(); }

		EntityID m_entity;
		std::tuple< typename Components::Ptr ... > m_componentPtrs;
	};

	const Result* Get( EntityID entity ) const
	{
		auto iter = std::lower_bound( begin(), end(), entity, []( const Result& result, EntityID entity ) {
			return result.ID() < entity;
		} );

		if ( iter == end() || iter->ID() != entity )
			return nullptr;

		return &*iter;
	}

	std::vector< Result >::const_iterator begin() const { return m_results.cbegin(); }
	std::vector< Result >::const_iterator end() const { return m_results.cend(); }

	void ClearResults() override { m_results.clear(); }
	void Consider( const World::EntityIterator& entity ) override
	{
		if ( Result result = Result( entity ) )
			m_results.push_back( entity );
	}

	void OnComponentAddedOrRemoved( size_t component_type_hash ) override
	{
		m_needsRerun = ( ( component_type_hash == typeid( typename Components::Type ).hash_code() ) || ... );
	}

	void CollectComponentTypes( std::set< size_t >& component_set ) override
	{
		component_set.insert( { typeid( typename Components::Type ).hash_code() ... } );
	}

private:
	std::vector< Result > m_results;
};

struct QuerySet
{
	QuerySet( World& world ) : m_world( world ) {}

	template< typename Query >
	std::shared_ptr< Query > Get()
	{
		const size_t query_type_hash = typeid( Query ).hash_code();

		auto iter = m_queries.find( query_type_hash );
		if ( iter == m_queries.end() )
			iter = m_queries.insert( { query_type_hash, {} } ).first;

		std::shared_ptr< IQuery > query = iter->second.lock();
		if ( !query )
			iter->second = query = m_world.m_queryManager.Get< Query >();

		return std::static_pointer_cast<Query>( query );
	}

	void Update();

private:

	World& m_world;
	std::map< size_t, std::weak_ptr< IQuery > > m_queries;
};

}
