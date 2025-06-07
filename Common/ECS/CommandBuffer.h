#pragma once

#include "World.h"

#include <deque>
#include <memory>
#include <mutex>

namespace onyx::ecs
{

struct ICommand
{
	virtual void Execute( World& world ) = 0;
};

template< typename ... Components >
struct AddEntityCommand : ICommand
{
	EntityID m_entity;
	std::tuple< Components ... > m_components;

	AddEntityCommand( EntityID entity, Components ... components )
		: m_entity( entity )
		, m_components( components )
	{}

	void Execute( World& world ) override
	{
		// kindly ignore this horrible incantation
		( [ & ]() {
			world.AddComponent( m_entity, std::get< Components >( m_components ) );
			return true;
		}() && ... );
	}
};

struct RemoveEntityCommand : ICommand
{
	EntityID m_entity;

	RemoveEntityCommand( EntityID entity ) : m_entity( entity ) {}

	void Execute( World& world ) override
	{
		world.RemoveEntity( m_entity );
	}
};

template< typename Component >
struct AddComponentCommand : ICommand
{
	EntityID m_entity;
	Component m_component;

	AddComponentCommand( EntityID entity, Component component ) : m_entity( entity ), m_component( component ) {}

	void Execute( World& world ) override
	{
		world.AddComponent( m_entity, m_component );
	}
};

template< typename Component >
struct RemoveComponentCommand : ICommand
{
	EntityID m_entity;

	RemoveComponentCommand( EntityID entity ) : m_entity( entity ) {}

	void Execute( World& world ) override
	{
		world.RemoveComponent< Component >( m_entity );
	}
};

struct CommandBuffer
{
	CommandBuffer( World& world ) : m_world( world ) {}

	template< typename ... Components >
	EntityID AddEntity( Components ... components )
	{
		std::scoped_lock lock( m_mutex );

		EntityID entity_id = m_world.AddEntity();

		m_commands.push_back( std::make_unique< AddEntityCommand< Components > >( entity_id, components ... ) );

		return entity_id;
	}

	void RemoveEntity( EntityID entity )
	{
		std::scoped_lock lock( m_mutex );
		m_commands.push_back( std::make_unique< RemoveEntityCommand >( entity ) );
	}

	template< typename Component >
	void AddComponent( EntityID entity, Component component )
	{
		std::scoped_lock lock( m_mutex );
		m_commands.push_back( std::make_unique< AddComponentCommand >( entity, component ) );
	}

	template< typename Component >
	void RemoveComponent( EntityID entity )
	{
		std::scoped_lock lock( m_mutex );
		m_commands.push_back( std::make_unique< RemoveComponentCommand >( entity ) );
	}

	void Execute()
	{
		while ( !m_commands.empty() )
		{
			m_commands.front()->Execute( m_world );
			m_commands.pop_front();
		}

		m_world.m_queryManager.UpdateNeedsRerun( m_world );
	}

private:

	std::mutex m_mutex;
	World& m_world;
	std::deque< std::unique_ptr< ICommand > > m_commands;
};

}
