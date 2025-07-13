#pragma once

#include "Scene.h"
#include "World.h"

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <tuple>

namespace onyx::ecs
{

struct ICommand
{
	virtual ~ICommand() {}
	virtual void Execute( World& world ) = 0;
};

template< typename ... Components >
struct AddEntityCommand : ICommand
{
	EntityID m_entity;
	std::tuple< Components ... > m_components;

	AddEntityCommand( EntityID entity, Components ... components )
		: m_entity( entity )
		, m_components( components ... )
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

template< typename Func >
struct CopySceneToWorldCommand : ICommand
{
	std::shared_ptr< Scene > m_scene;
	Func m_func;

	CopySceneToWorldCommand( std::shared_ptr< Scene > scene, Func func )
		: m_scene( scene )
		, m_func( func )
	{
		WEAK_ASSERT( m_scene->GetLoadingState() != LoadingState::Errored );
	}

	void Execute( World& world ) override
	{
		if ( m_scene->GetLoadingState() == LoadingState::Unloaded )
			m_scene->Load( IAsset::LoadType::Stream );

		if ( !WEAK_ASSERT( m_scene->GetLoadingState() == LoadingState::Loaded ) )
			return;
		
		IDMap entity_id_mapping;
		m_scene->CopyToWorld( world, entity_id_mapping );
		m_func( world, entity_id_mapping );
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

		m_commands.push_back( std::make_unique< AddEntityCommand< Components ... > >( entity_id, components ... ) );

		return entity_id;
	}

	void RemoveEntity( EntityID entity )
	{
		std::scoped_lock lock( m_mutex );
		m_commands.push_back( std::make_unique< RemoveEntityCommand >( entity ) );
	}

	template< typename Component >
	Component& AddComponent( EntityID entity, Component component )
	{
		std::scoped_lock lock( m_mutex );
		std::unique_ptr< AddComponentCommand< Component > > cmd = std::make_unique< AddComponentCommand< Component > >( entity, component );
		Component& result = cmd->m_component;
		m_commands.push_back( std::move( cmd ) );
		return result;
	}

	template< typename Component >
	void RemoveComponent( EntityID entity )
	{
		std::scoped_lock lock( m_mutex );
		m_commands.push_back( std::make_unique< RemoveComponentCommand >( entity ) );
	}

	static void IgnorePostCopySceneToWorld( World&, IDMap& ) {}

	template< typename Func = void(*)( World&, IDMap& ) >
	void CopySceneToWorld( std::shared_ptr< Scene > scene, Func func = IgnorePostCopySceneToWorld )
	{
		std::scoped_lock lock( m_mutex );
		m_commands.push_back( std::make_unique< CopySceneToWorldCommand< Func > >( scene, func ) );
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
