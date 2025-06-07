#pragma once

#include "Query.h"
#include "World.h"
#include "CommandBuffer.h"

#include <tuple>
#include <memory>

namespace onyx::ecs
{

template< typename Global >
struct ISystem
{
	virtual void Run( Global& global ) const = 0;
};

template< typename Global, typename Func >
struct System;

template< typename Global, typename ... Queries >
struct System< Global, void( Global&, const Queries& ... ) > : ISystem< Global >
{
	using Func = void( Global&, const Queries& ... );
	Func* const m_callback;

	System( QuerySet& query_set, Func* callback )
		: m_callback( callback )
		, m_queries( query_set.Get< Queries >() ... )
	{}

	void Run( Global& global ) const override { ( *m_callback )( global, *std::get< std::shared_ptr< Queries > >( m_queries ) ... ); }

private:

	std::tuple< std::shared_ptr< Queries > ... > m_queries;
};

}
