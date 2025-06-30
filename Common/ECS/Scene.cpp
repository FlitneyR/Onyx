#include "Scene.h"

namespace onyx::ecs
{

ComponentReflectorTable ComponentReflectorTable::s_singleton {};

const IComponentReflector* ComponentReflectorTable::GetReflector( size_t component_type_hash ) const
{
	const auto iter = m_typeHashLookup.find( component_type_hash );
	if ( iter == m_typeHashLookup.end() )
		return nullptr;

	return &iter->second;
}

const IComponentReflector* ComponentReflectorTable::GetReflector( BjSON::NameHash component_name_hash ) const
{
	const auto iter = m_typeNameLookup.find( component_name_hash );
	if ( iter == m_typeNameLookup.end() )
		return nullptr;
	
	return &iter->second;
}

void ComponentReflectorTable::RegisterReflector( const IComponentReflector& reflector, BjSON::NameHash component_name_hash, size_t component_type_hash )
{
	m_typeHashLookup.insert( { component_type_hash, reflector } );
	m_typeNameLookup.insert( { component_name_hash, reflector } );
	m_reflectors.push_back( reflector );
}

void ComponentReflectorTable::SerialiseDiffs( BjSON::IReadWriteObject& writer, World& world, EntityID entity, World& src_world, EntityID src_entity )
{
	for ( auto& reflector : m_reflectors )
		reflector.SerialiseDiff( writer, world, entity, src_world, src_entity );
}

void Scene::CopyToWorld( World& world, std::vector< std::pair< EntityID, EntityID > >& map ) const
{
	for ( auto iter = m_world.Iter(); iter; ++iter )
		map.push_back( { iter.ID(), iter.CopyToWorld( world ) } );
}

void Scene::Load( LoadType type )
{
	m_loadingState = LoadingState::Loading;

	const BjSON::IReadOnlyObject* const reader = GetReader();
	if ( !reader || !WEAK_ASSERT( reader->GetLiteral< u32 >( "__assetType"_name ) == "Scene"_name ) )
		RETURN_LOAD_ERRORED();

	auto entity_reader = reader->GetArray( "Entities"_name );
	if ( !WEAK_ASSERT( entity_reader ) )
		RETURN_LOAD_ERRORED();

	// recycle this to (hopefully) reduce memory thrashing
	std::vector< std::pair< EntityID, EntityID > > entity_map;

	for ( u32 entity_idx = 0; entity_idx < entity_reader->Count(); ++entity_idx )
	{
		auto entity = entity_reader->GetChild( entity_idx );

		EntityID id;
		if ( !WEAK_ASSERT( entity->GetLiteral( "ID"_name, id ) == sizeof( id ) ) )
			RETURN_LOAD_ERRORED();

		if ( auto scene_instance = entity->GetChild( "SceneInstance"_name ) )
		{
			const std::string sub_scene_path = scene_instance->GetLiteral< std::string >( "Scene"_name );
			std::shared_ptr< Scene > scene = m_assetManager->Load< Scene >( sub_scene_path, false, type );

			if ( !WEAK_ASSERT( scene && scene->GetLoadingState() == LoadingState::Loaded ) )
				RETURN_LOAD_ERRORED();

			const EntityID scene_root = id;
			m_world.AddComponent( scene_root, SceneInstance( scene, scene_root ) );

			scene->CopyToWorld( m_world, entity_map );

			// we don't need this information in game, only in editor
			if ( type == LoadType::Editor )
				for ( const auto& [ scene_id, world_id ] : entity_map )
					m_world.AddComponent( world_id, SceneInstance( scene, scene_root, scene_id ) );

			// apply diffs
			if ( auto entity_diffs = WEAK_ASSERT( entity->GetArray( "Diffs"_name ) ) )
			{
				for ( u32 entity_diff_idx = 0; entity_diff_idx < entity_diffs->Count(); ++entity_diff_idx )
				{
					auto entity_diff = entity_diffs->GetChild( entity_diff_idx );
					auto component_overrides = WEAK_ASSERT( entity_diff->GetChild( "Components"_name ) );

					// find the world entity id for this diff
					EntityID scene_id = NoEntity;
					if ( !WEAK_ASSERT( entity_diff->GetLiteral( "SceneID"_name, scene_id ) == sizeof( scene_id ) && scene_id != NoEntity ) )
						RETURN_LOAD_ERRORED();

					EntityID world_id = NoEntity;
					for ( const auto& [sid, wid] : entity_map )
						if ( sid == scene_id ) { world_id = wid; break; }

					if ( !WEAK_ASSERT( world_id != NoEntity ) )
						RETURN_LOAD_ERRORED();

					// apply the overrides
					for ( u32 component_override_idx = 0; component_override_idx < component_overrides->GetMemberCount(); ++component_override_idx )
					{
						const BjSON::NameHash component_name = component_overrides->GetMemberName( component_override_idx );

						if ( const IComponentReflector* const reflector = WEAK_ASSERT( ComponentReflectorTable::s_singleton.GetReflector( component_name ) ) )
							reflector->DeserialiseComponent( *component_overrides, m_world, world_id );
						else
							RETURN_LOAD_ERRORED();
					}
				}
			}

			entity_map.clear();
		}
		else
		{
			for ( u32 component_idx = 0; component_idx < entity->GetMemberCount(); ++component_idx )
				if ( const IComponentReflector* const reflector = ComponentReflectorTable::s_singleton.GetReflector( entity->GetMemberName( component_idx ) ) )
					reflector->DeserialiseComponent( *entity, m_world, id );
		}
	}

	if ( !WEAK_ASSERT( reader->GetLiteral( "NextEntityID"_name, m_world.m_nextEntityID ) == sizeof( EntityID ) ) )
		RETURN_LOAD_ERRORED();

	m_loadingState = LoadingState::Loaded;
}

void Scene::Save( BjSON::IReadWriteObject& writer, SaveType type )
{
	writer.SetLiteral( "__assetType"_name, "Scene"_name );
	writer.SetLiteral( "NextEntityID"_name, m_world.m_nextEntityID );

	// recycle this to (hopefully) reduce memory thrashing
	std::vector< std::pair< EntityID, EntityID > > entity_map;

	auto& entities_writer = writer.AddArray( "Entities"_name );
	for ( auto entity_iter = m_world.Iter(); entity_iter; ++entity_iter )
	{
		auto& entity_writer = entities_writer.AddChild();
		entity_writer.SetLiteral( "ID"_name, entity_iter.ID() );

		if ( const SceneInstance* const scene_instance = entity_iter.Get< SceneInstance >() )
		{
			const EntityID scene_root = entity_iter.ID();
			if ( !WEAK_ASSERT( scene_instance->m_scene && scene_instance->m_scene->GetLoadingState() == LoadingState::Loaded ) )
				return;

			// collect the children of this scene instance, and which entities from the scene they are copies of
			while ( entity_iter )
			{
				if ( const SceneInstance* const scene_child = entity_iter.Get< SceneInstance >() )
				{
					if ( scene_child->m_rootEntity != scene_root )
						break;

					entity_map.push_back( { scene_child->m_sceneEntityId, entity_iter.ID() } );
				}
				else
					break;
			}

			auto& scene_instance_writer = entity_writer.AddChild( "SceneInstance"_name );
			scene_instance_writer.SetLiteral( "Scene"_name, scene_instance->m_scene->m_path );

			auto& diffs_writer = scene_instance_writer.AddArray( "Diffs"_name );

			for ( const auto& [scene_entity_id, world_entity_id] : entity_map )
			{
				auto& diff_writer = diffs_writer.AddChild();
				diff_writer.SetLiteral( "SceneID"_name, scene_entity_id );
				
				auto& components_writer = diff_writer.AddChild( "Components"_name );
				ComponentReflectorTable::s_singleton.SerialiseDiffs( components_writer, m_world, world_entity_id, scene_instance->m_scene->m_world, scene_entity_id );
			}

			entity_map.clear();
		}
		else
		{
			for ( auto& iter : entity_iter.m_iterators )
				if ( iter.second->ID() == entity_iter.ID() )
					if ( auto reflector = ComponentReflectorTable::s_singleton.GetReflector( iter.first ) )
						reflector->SerialiseComponent( entity_writer, entity_iter );
		}
	}
}

void Scene::DoAssetManagerButton( const char* name, const char* path, f32 width, std::shared_ptr< IAsset > asset, IFrameContext& frame_context )
{
	switch ( m_loadingState )
	{
	case LoadingState::Loaded:
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.f, 0.25f, 0.0f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.f, 0.5f, 0.0f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.f, 0.125f, 0.0f, 1.f ) );
		break;
	case LoadingState::Errored:
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.25f, 0.f, 0.f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.5f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.125f, 0.f, 0.f, 1.f ) );
		break;
	default:
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.25f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.25f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.125f, 0.125f, 0.125f, 1.f ) );
		break;
	}

	if ( ImGui::Button( name ) )
	{
		if ( m_loadingState == LoadingState::Unloaded )
			Load( IAsset::LoadType::Editor );

		SceneEditor* const window = editor::AddWindow< SceneEditor >();
		window->m_scene = std::static_pointer_cast< Scene >( asset );
	}

	ImGui::PopStyleColor( 3 );
}

void SceneEditor::Run( IFrameContext& frame_context )
{
	if ( ImGui::Begin( GetWindowTitle().c_str(), &m_open ) )
	{
	}

	ImGui::End();
}

std::string SceneEditor::GetWindowTitle() const
{
	return std::format( "{}: {}###{}", s_name, !m_scene ? "" : m_scene->m_path, (u64)this );
}

}
