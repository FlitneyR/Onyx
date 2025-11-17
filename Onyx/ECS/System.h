#pragma once

#include "Query.h"
#include "World.h"
#include "CommandBuffer.h"

#include <tuple>
#include <memory>

namespace onyx::ecs
{

template< typename IContext >
struct ISystem
{
	ISystem( u64 id ) : id( id ) {}

	virtual ~ISystem() = default;

	virtual void Run( const IContext& context ) const = 0;

	u64 GetID() const { return id; }

private:
	u64 id = 0;
};

template< typename IContext, typename Func >
struct System;

template< typename IContext, typename Context, typename ... Queries >
struct System< IContext, void( Context, const Queries& ... ) > : ISystem< IContext >
{
	using Func = void( Context, const Queries& ... );

	System( QuerySet& query_set, Func* callback )
		: ISystem< IContext >( (u64)callback )
		, m_callback( callback )
		, m_queries( query_set.Get< Queries >() ... )
	{}

	void Run( const IContext& context ) const override
	{
		( *m_callback )( Context( context ), *std::get< std::shared_ptr< Queries > >( m_queries ) ... );
	}

private:
	Func* const m_callback;
	std::tuple< std::shared_ptr< Queries > ... > m_queries;
};

}
