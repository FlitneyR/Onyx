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

template< typename Arg > struct ComponentType;
template< typename T > using ComponentTypeT = typename ComponentType< T >::Type;

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

template< typename T > struct ComponentType< const T& > { using Type = Read< T >; };
template< typename T > struct ComponentType<       T& > { using Type = Write< T >; };
template< typename T > struct ComponentType< const T* > { using Type = ReadOptional< T >; };
template< typename T > struct ComponentType<       T* > { using Type = WriteOptional< T >; };

template< typename ... Components >
struct Context
{
	Context( const Context& other ) = default;

	Context( Components& ... components )
		: m_components( components ... )
	{}

	template< typename OtherContext >
	Context( OtherContext& other_context )
		: m_components( other_context.template Get< Components >() ... )
	{}

	template< typename T >
	T& Get() const
	{
		static_assert( s_hasComponent< T > || s_hasComponent< std::remove_const_t< T > >, "Missing context component" );
		return std::get< ChooseTypeT< s_hasComponent< std::remove_const_t< T > >, std::remove_const_t< T >&, T& > >( m_components );
	}

	std::tuple< Components& ... > Break() const { return m_components; }

private:
	template< typename ... OtherComponents >
	friend struct Context;

	template< typename T > static constexpr bool s_hasComponent = ( std::is_same_v< T, Components > || ... );

	template< bool b, typename T1, typename T2 > struct ChooseType;
	template< typename T1, typename T2 > struct ChooseType< true, T1, T2 > { using Type = T1; };
	template< typename T1, typename T2 > struct ChooseType< false, T1, T2 > { using Type = T2; };

	template< bool b, typename T1, typename T2 > using ChooseTypeT = ChooseType< b, T1, T2 >::Type;

	std::tuple< Components& ... > m_components;
};

template< typename ... Components >
struct Query : IQuery
{
	struct Result
	{
		using Query = Query;

		Result( const World::EntityIterator& entity )
			: m_entity( entity.GetEntityID() )
			, m_componentPtrs( entity.Get< typename Components::Type >() ... )
		{}

		EntityID GetEntityID() const { return m_entity; }

		template< typename T >
		T Get() const
		{
			using Component = ComponentTypeT< T >;
			static_assert( ( std::is_same_v< Component, Components > || ... ), "Missing entity component" );
			return Component::Cast( std::get< typename Component::Ptr >( m_componentPtrs ) );
		}

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
			return result.GetEntityID() < entity;
		} );

		if ( iter == end() || iter->GetEntityID() != entity )
			return nullptr;

		return &*iter;
	}

	u32 Count() const { return static_cast< u32 >( m_results.size() ); }
	const Result& operator []( u32 index ) const { return m_results[ index ]; }

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
		m_needsRerun |= ( ( component_type_hash == typeid( typename Components::Type ).hash_code() ) || ... );
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
