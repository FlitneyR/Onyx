#pragma once

#include "World.h"

namespace onyx::ecs
{

struct IQuery
{
	virtual void ClearResults() = 0;
	virtual void Consider( const World::EntityIterator& entity ) = 0;
	virtual void OnComponentAddedOrRemoved( size_t component_type_hash ) = 0;

	bool NeedsRerun() const { return m_needsRerun; }
	void ResetNeedsRerun() { m_needsRerun = false; }

protected:
	bool m_needsRerun = false;
};

template< typename ... Components >
struct Query : IQuery
{
	struct Result
	{
		using Query = Query;

		Result( const World::EntityIterator& entity )
			: m_entity( entity.ID() )
			, m_components( entity.Get< Components >() ... )
		{}

		EntityID ID() const { return m_entity; }

		template< typename Component >
		Component& Get() const { return *std::get< Component* >( m_components ); }

		std::tuple< Components& ... > Break() const { return { Get< Components >() ... }; }

	private:
		friend Query;
		bool IsComplete() const { return ( std::get< Components* >( m_components ) && ... ); }
		operator bool() const { return IsComplete(); }

		EntityID m_entity;
		std::tuple< Components* ... > m_components;
	};

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
		m_needsRerun = ( ( component_type_hash == typeid( Components ).hash_code() ) || ... );
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
