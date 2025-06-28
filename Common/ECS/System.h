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
	virtual void Run( IContext& context ) const = 0;
};

template< typename IContext, typename Func >
struct System;

template< typename IContext, typename Context, typename ... Queries >
struct System< IContext, void( Context, const Queries& ... ) > : ISystem< IContext >
{
	using Func = void( Context, const Queries& ... );

	System( QuerySet& query_set, Func* callback )
		: m_callback( callback )
		, m_queries( query_set.Get< Queries >() ... )
	{}

	void Run( IContext& context ) const override { ( *m_callback )( Context( context ), *std::get< std::shared_ptr< Queries > >( m_queries ) ... ); }

private:
	Func* const m_callback;
	std::tuple< std::shared_ptr< Queries > ... > m_queries;
};

}
