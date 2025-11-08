#include "Scene.h"
#include "Onyx/ECS/Modules/Core.h"
#include "Onyx/LowLevel/LowLevelInterface.h"

#include "Onyx/Graphics/RenderTarget.h"

#include "imgui_stdlib.h"

namespace onyx::ecs
{

ComponentReflectorTable ComponentReflectorTable::s_singleton {};

DEFINE_DEFAULT_SERIALISE_PROPERTY( EntityID ) { writer.SetLiteral( name, value ); }
DEFINE_DEFAULT_DESERIALISE_PROPERTY( EntityID ) { reader.GetLiteral( name, value ); }

DEFINE_DEFAULT_PROPERTY_EDITOR_UI( EntityID )
{
	// ImGui::DragInt( name, (int*)&value, 1.f, 0, INT_MAX );
	ImGuiScopedID scoped_id( name );

	std::string name_str = std::format( "{}", value );
	if ( const Core::Name* name_comp = world.GetComponent< Core::Name >( value ) )
		name_str = std::format( "{}({})", name_comp->name, value );

	if ( ImGui::Button( std::format( "{} : {}", name, name_str.c_str() ).c_str() ) )
		ImGui::OpenPopup( "Entity Picker" );

	bool has_changed = false;
	if ( ImGui::BeginPopup( "Entity Picker" ) )
	{
		static std::string search_term;
		ImGui::InputText( "Name", &search_term );

		for ( auto iter = world.Iter(); iter; ++iter )
		{
			std::string entity_name = std::format( "{}", iter.ID() );

			if ( const Core::Name* name = iter.Get< Core::Name >() )
			{
				if ( !strstr( name->name.c_str(), search_term.c_str() ) )
					continue;

				entity_name = std::format( "{}({})", name->name, iter.ID() );
			}

			if ( ImGui::Selectable( entity_name.c_str() ) )
			{
				has_changed = true;
				value = iter.ID();
			}
		}

		ImGui::EndPopup();
	}

	return has_changed;
}

DEFINE_DEFAULT_PROPERTY_DIFF_HINT( EntityID )
{
	if ( const Core::Name* const name = src_world.GetComponent< Core::Name >( value ) )
		ImGui::SetTooltip( "%s(%d)", name->name.c_str(), value );

	ImGui::SetTooltip( "%d", value );
}

DEFINE_DEFAULT_SERIALISE_PROPERTY( bool ) { writer.SetLiteral( name, value ); }
DEFINE_DEFAULT_DESERIALISE_PROPERTY( bool ) { reader.GetLiteral( name, value ); }
DEFINE_DEFAULT_PROPERTY_EDITOR_UI( bool ) { return ImGui::Checkbox( name, &value ); }
DEFINE_DEFAULT_PROPERTY_DIFF_HINT( bool ) { ImGui::SetTooltip( value ? "true" : "false" ); }

DEFINE_DEFAULT_SERIALISE_PROPERTY( u32 ) { writer.SetLiteral( name, value ); }
DEFINE_DEFAULT_DESERIALISE_PROPERTY( u32 ) { reader.GetLiteral( name, value ); }
DEFINE_DEFAULT_PROPERTY_EDITOR_UI( u32 ) { return ImGui::DragInt( name, (int*)&value, 1.f, 0, INT_MAX ); }
DEFINE_DEFAULT_PROPERTY_DIFF_HINT( u32 ) { ImGui::SetTooltip( "%d", value ); }

DEFINE_DEFAULT_SERIALISE_PROPERTY( i32 ) { writer.SetLiteral( name, value ); }
DEFINE_DEFAULT_DESERIALISE_PROPERTY( i32 ) { reader.GetLiteral( name, value ); }
DEFINE_DEFAULT_PROPERTY_EDITOR_UI( i32 ) { return ImGui::DragInt( name, &value ); }
DEFINE_DEFAULT_PROPERTY_DIFF_HINT( i32 ) { ImGui::SetTooltip( "%d", value ); }

DEFINE_DEFAULT_SERIALISE_PROPERTY( f32 ) { writer.SetLiteral( name, value ); }
DEFINE_DEFAULT_DESERIALISE_PROPERTY( f32 ) { reader.GetLiteral( name, value ); }
DEFINE_DEFAULT_PROPERTY_EDITOR_UI( f32 ) { return ImGui::DragFloat( name, &value, 0.1f ); }
DEFINE_DEFAULT_PROPERTY_DIFF_HINT( f32 ) { ImGui::SetTooltip( "%f", value ); }

DEFINE_DEFAULT_SERIALISE_PROPERTY( std::string ) { writer.SetLiteral( name, value ); }
DEFINE_DEFAULT_DESERIALISE_PROPERTY( std::string ) { reader.GetLiteral( name, value ); }
DEFINE_DEFAULT_PROPERTY_EDITOR_UI( std::string ) { return ImGui::InputText( name, &value ); }
DEFINE_DEFAULT_PROPERTY_DIFF_HINT( std::string ) { ImGui::SetTooltip( "%s", value.c_str() ); }

DEFINE_DEFAULT_SERIALISE_PROPERTY( glm::vec2 ) { writer.SetLiteral( name, value ); }
DEFINE_DEFAULT_DESERIALISE_PROPERTY( glm::vec2 ) { reader.GetLiteral( name, value ); }
DEFINE_DEFAULT_PROPERTY_EDITOR_UI( glm::vec2 ) { return ImGui::DragFloat2( name, &value.x, 0.1f ); }
DEFINE_DEFAULT_PROPERTY_DIFF_HINT( glm::vec2 ) { ImGui::SetTooltip( "%f, %f", value.x, value.y ); }

DEFINE_DEFAULT_SERIALISE_PROPERTY( glm::vec3 ) { writer.SetLiteral( name, value ); }
DEFINE_DEFAULT_DESERIALISE_PROPERTY( glm::vec3 ) { reader.GetLiteral( name, value ); }
DEFINE_DEFAULT_PROPERTY_EDITOR_UI( glm::vec3 ) { return ImGui::DragFloat3( name, &value.x, 0.1f ); }
DEFINE_DEFAULT_PROPERTY_DIFF_HINT( glm::vec3 ) { ImGui::SetTooltip( "%f, %f, %f", value.x, value.y, value.z ); }

DEFINE_DEFAULT_SERIALISE_PROPERTY( std::shared_ptr< Scene > ) { writer.SetLiteral( name, !value ? "" : value->m_path ); }
DEFINE_DEFAULT_PROPERTY_DIFF_HINT( std::shared_ptr< Scene > ) { ImGui::SetTooltip( "%s", !value ? "" : value->m_path ); }
DEFINE_DEFAULT_DESERIALISE_PROPERTY( std::shared_ptr< Scene > )
{ value = asset_manager.Load< Scene >( reader.GetLiteral< std::string >( name ) ); }

DEFINE_DEFAULT_PROPERTY_EDITOR_UI( std::shared_ptr< Scene > )
{
	ImGuiScopedID scoped_id( name );

	if ( ImGui::Button( std::format( "{}: {}", name, !value ? "" : value->m_path ).c_str() ) )
		ImGui::OpenPopup( "Select Scene" );

	bool was_edited = false;
	if ( ImGui::BeginPopup( "Select Scene" ) )
	{
		static std::string new_scene_path;
		ImGui::InputText( "Path", &new_scene_path );

		if ( ImGui::Button( "Cancel" ) )
			ImGui::CloseCurrentPopup();

		ImGui::SameLine();
		if ( ImGui::Button( "Ok" ) )
		{
			if ( new_scene_path.empty() )
			{
				was_edited = true;
				value = nullptr;
			}
			else if ( auto scene = asset_manager.Load< Scene >( new_scene_path, true, onyx::IAsset::LoadType::Editor ) )
			{
				was_edited = true;
				value = scene;
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}

	return was_edited;
}

const IComponentReflector* ComponentReflectorTable::GetReflector( size_t component_type_hash ) const
{
	const auto iter = m_typeHashLookup.find( component_type_hash );
	if ( iter == m_typeHashLookup.end() )
		return nullptr;

	return iter->second;
}

const IComponentReflector* ComponentReflectorTable::GetReflector( BjSON::NameHash component_name_hash ) const
{
	const auto iter = m_typeNameLookup.find( component_name_hash );
	if ( iter == m_typeNameLookup.end() )
		return nullptr;
	
	return iter->second;
}

void ComponentReflectorTable::RegisterReflector( const IComponentReflector& reflector, BjSON::NameHash component_name_hash, size_t component_type_hash )
{
	m_typeHashLookup.insert( { component_type_hash, &reflector } );
	m_typeNameLookup.insert( { component_name_hash, &reflector } );
	m_reflectors.push_back( &reflector );

	std::sort( m_reflectors.begin(), m_reflectors.end(), []( const IComponentReflector* lhs, const IComponentReflector* rhs )
	{
		return strcmp( lhs->m_name, rhs->m_name ) < 0;
	} );
}

void ComponentReflectorTable::SerialiseEdits( BjSON::IReadWriteObject& writer, const std::map< BjSON::NameHash, std::set< BjSON::NameHash > >& edits, World& world, EntityID entity )
{
	for ( auto& [component, edits] : edits )
		if ( !edits.empty() )
			if ( auto reflector = GetReflector( component ) )
				reflector->SerialiseEdits( writer, edits, world, entity );
}

void ComponentReflectorTable::DoEditorUI( AssetManager& asset_manager, World& world, EntityID entity )
{
	if ( !entity )
		return;

	if ( ImGui::BeginMenuBar() )
	{
		if ( ImGui::BeginMenu( "Add Component" ) )
		{
			static std::string search_term;

			for ( auto& reflector : m_reflectors )
				reflector->DoAddComponentButton( world, entity, search_term );

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	for ( auto& reflector : m_reflectors )
		reflector->DoEditorUI( asset_manager, world, entity );
}

void ComponentReflectorTable::PostCopyToWorld( World& world, EntityID entity, const IDMap& entity_id_map )
{
	for ( auto& reflector : m_reflectors )
		reflector->PostCopy( world, entity, entity_id_map );
}

void Scene::CopyToWorld( World& world, IDMap& map ) const
{
	for ( auto iter = m_world.Iter(); iter; ++iter )
		map.push_back( { iter.ID(), iter.CopyToWorld( world ) } );

	for ( const auto& [src_id, dst_id] : map )
		ComponentReflectorTable::s_singleton.PostCopyToWorld( world, dst_id, map );
}

void Scene::Load( LoadType type )
{
	ZoneScoped;

	m_loadingState = LoadingState::Loading;

	const BjSON::IReadOnlyObject* const reader = GetReader();
	if ( !reader || !WEAK_ASSERT( reader->GetLiteral< u32 >( "__assetType"_name ) == "Scene"_name ) )
		RETURN_LOAD_ERRORED();

	auto entities_reader = reader->GetArray( "Entities"_name );
	if ( !WEAK_ASSERT( entities_reader ) )
		RETURN_LOAD_ERRORED();

	// recycle this to reduce memory thrashing
	IDMap entity_map;

	for ( u32 entity_idx = 0; entity_idx < entities_reader->Count(); ++entity_idx )
	{
		auto entity_reader = entities_reader->GetChild( entity_idx );

		EntityID id;
		if ( !WEAK_ASSERT( entity_reader->GetLiteral( "ID"_name, id ) ) )
			RETURN_LOAD_ERRORED();

		if ( auto scene_instance_reader = entity_reader->GetChild( "SceneInstance"_name ) )
		{
			const std::string scene_path = scene_instance_reader->GetLiteral< std::string >( "Scene"_name );
			std::shared_ptr< Scene > scene = m_assetManager->Load< Scene >( scene_path, false, type );

			if ( !WEAK_ASSERT( scene && scene->GetLoadingState() == LoadingState::Loaded ) )
				RETURN_LOAD_ERRORED();

			const EntityID scene_root = id;
			WEAK_ASSERT( m_world.m_nextEntityID <= scene_root );
			m_world.m_nextEntityID = scene_root;

			if ( type == LoadType::Editor )
			{
				m_world.AddEntity(
					SceneInstance( scene, scene_root ),
					Core::Name( std::format( "Scene Instance: {}", scene->m_path ) )
				);
			}
			else
			{
				m_world.AddEntity();
			}

			scene->CopyToWorld( m_world, entity_map );

			// we don't need this information in game, only in editor
			if ( type == LoadType::Editor )
				for ( const auto& [ scene_id, world_id ] : entity_map )
					m_world.AddComponent( world_id, SceneInstance( scene, scene_root, scene_id ) );

			// apply diffs
			if ( auto diffs_reader = WEAK_ASSERT( scene_instance_reader->GetArray( "Diffs"_name ) ) )
			{
				for ( u32 entity_diff_idx = 0; entity_diff_idx < diffs_reader->Count(); ++entity_diff_idx )
				{
					auto diff_reader = diffs_reader->GetChild( entity_diff_idx );
					auto components_reader = WEAK_ASSERT( diff_reader->GetChild( "Components"_name ) );

					// find the world entity id for this diff
					EntityID scene_id = NoEntity;
					if ( !WEAK_ASSERT( diff_reader->GetLiteral( "SceneID"_name, scene_id ) && scene_id != NoEntity ) )
						RETURN_LOAD_ERRORED();

					EntityID world_id = NoEntity;
					for ( const auto& [sid, wid] : entity_map )
						if ( sid == scene_id ) { world_id = wid; break; }

					SceneInstance* const entity_scene_instance = m_world.GetComponent< SceneInstance >( world_id );

					if ( !WEAK_ASSERT( world_id != NoEntity ) )
						RETURN_LOAD_ERRORED();

					// apply the overrides
					for ( u32 component_override_idx = 0; component_override_idx < components_reader->GetMemberCount(); ++component_override_idx )
					{
						const BjSON::NameHash component_name = components_reader->GetMemberName( component_override_idx );
						auto component_reader = components_reader->GetChild( component_name );
						
						if ( entity_scene_instance )
						{
							auto& properties = entity_scene_instance->m_edits.insert( { component_name, {} } ).first->second;

							for ( u32 property_idx = 0; property_idx < component_reader->GetMemberCount(); ++property_idx )
								properties.insert( component_reader->GetMemberName( property_idx ) );
						}

						if ( const IComponentReflector* const reflector = WEAK_ASSERT( ComponentReflectorTable::s_singleton.GetReflector( component_name ) ) )
							reflector->DeserialiseComponent( *component_reader, *m_assetManager, m_world, world_id );
						else
							RETURN_LOAD_ERRORED();
					}
				}
			}

			entity_map.clear();
		}
		else
		{
			for ( u32 component_idx = 0; component_idx < entity_reader->GetMemberCount(); ++component_idx )
				if ( const IComponentReflector* const reflector = ComponentReflectorTable::s_singleton.GetReflector( entity_reader->GetMemberName( component_idx ) ) )
					reflector->DeserialiseComponent( *entity_reader->GetChild( entity_reader->GetMemberName( component_idx ) ), *m_assetManager, m_world, id );
		}
	}

	if ( !WEAK_ASSERT( reader->GetLiteral( "NextEntityID"_name, m_world.m_nextEntityID ) ) )
		RETURN_LOAD_ERRORED();

	m_loadingState = LoadingState::Loaded;
}

void Scene::Save( BjSON::IReadWriteObject& writer, SaveType type )
{
	writer.SetLiteral( "__assetType"_name, "Scene"_name );
	writer.SetLiteral( "NextEntityID"_name, m_world.m_nextEntityID );

	// recycle this to (hopefully) reduce memory thrashing
	IDMap entity_map;

	auto& entities_writer = writer.AddArray( "Entities"_name );
	for ( auto entity_iter = m_world.Iter(); entity_iter; )
	{
		auto& entity_writer = entities_writer.AddChild();
		entity_writer.SetLiteral( "ID"_name, entity_iter.ID() );

		if ( const SceneInstance* const scene_instance = entity_iter.Get< SceneInstance >() )
		{
			const EntityID scene_root = entity_iter.ID();
			if ( !WEAK_ASSERT( scene_instance->m_scene && scene_instance->m_scene->GetLoadingState() == LoadingState::Loaded ) )
				return;

			// collect the children of this scene instance, and which entities from the scene they are copies of
			while ( ++entity_iter )
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

				const SceneInstance* const entity_scene_instance = m_world.GetComponent< SceneInstance >( world_entity_id );
				if ( !WEAK_ASSERT( entity_scene_instance, "Entity {} doesn't have a SceneInstance component, so its edits can't be saved", world_entity_id ) )
					continue;
				
				auto& components_writer = diff_writer.AddChild( "Components"_name );
				ComponentReflectorTable::s_singleton.SerialiseEdits( components_writer, entity_scene_instance->m_edits, m_world, world_entity_id );
			}

			entity_map.clear();
		}
		else
		{
			for ( auto& iter : entity_iter.m_iterators )
				if ( iter.second->ID() == entity_iter.ID() )
					if ( auto reflector = ComponentReflectorTable::s_singleton.GetReflector( iter.first ) )
						reflector->SerialiseComponent( entity_writer, entity_iter );

			++entity_iter;
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
		window->m_previewer = !SceneEditor::IPreviewer::IFactory::s_singleton ? nullptr
			: SceneEditor::IPreviewer::IFactory::s_singleton->MakePreviewer( m_world );
	}

	ImGui::PopStyleColor( 3 );
}

void SceneEditor::Run( IFrameContext& frame_context )
{
	ZoneScoped;

	if ( ImGui::Begin( GetWindowTitle().c_str(), &m_open, ImGuiWindowFlags_MenuBar ) )
	{
		ImGui::SetWindowSize( { 1280, 720 }, ImGuiCond_Once );

		ImVec2 work_size = ImGui::GetWindowSize();
		work_size.x = std::max( 10.f, work_size.x - 10 );
		work_size.y = std::max( 10.f, work_size.y - 60 );

		if ( !m_renderTarget || m_renderTarget->GetSize().x != work_size.x || m_renderTarget->GetSize().y != work_size.y )
			m_renderTarget = onyx::LowLevel::GetGraphicsContext().CreateRenderTarget( { work_size.x, work_size.y } );

		m_scene->m_world.m_queryManager.UpdateNeedsRerun( m_scene->m_world );
		m_previewer->Tick( frame_context, m_renderTarget );
		m_renderTarget->PrepareForSampling( frame_context );

		frame_context.RegisterUsedResource( m_renderTarget );
		ImGui::Image( m_renderTarget->GetImTextureID(), work_size );
	}

	std::unordered_map< EntityID, std::vector< EntityID > > hierarchy;
	std::vector< EntityID > root_entities;

	for ( auto entity = m_scene->m_world.Iter(); entity; ++entity )
	{
		if ( const Core::AttachedTo* const attachment = entity.Get< Core::AttachedTo >() )
		{
			auto iter = hierarchy.find( attachment->localeEntity );
			if ( iter == hierarchy.end() )
			{
				hierarchy.insert( { attachment->localeEntity, {} } );
				iter = hierarchy.find( attachment->localeEntity );
			}

			iter->second.push_back( entity.ID() );
		}
		else
		{
			root_entities.push_back( entity.ID() );
		}
	}

	if ( ImGui::Begin( std::format( "Entity Inspector: {}###{}", m_scene->m_path, (u64)(this + 1) ).c_str(), &m_open, ImGuiWindowFlags_MenuBar ) )
	{
		ImGui::SetWindowSize( { 320, 720 }, ImGuiCond_Once );

		bool skip_component_editors = false;

		if ( const SceneInstance* const si = m_scene->m_world.GetComponent< SceneInstance >( m_selectedEntity ) )
		{
			if ( WEAK_ASSERT( si->m_scene && si->m_scene->GetLoadingState() == LoadingState::Loaded ) )
			{
				const Core::Name* const name = si->m_scene->m_world.GetComponent< Core::Name >( si->m_sceneEntityId );

				if ( ImGui::Button( std::format( "{}: {}({})", si->m_scene->m_path, !name ? "" : name->name, si->m_sceneEntityId ).c_str() ) )
				{
					m_scene = si->m_scene;
					m_selectedEntity = si->m_sceneEntityId;
					m_previewer = !SceneEditor::IPreviewer::IFactory::s_singleton ? nullptr
						: SceneEditor::IPreviewer::IFactory::s_singleton->MakePreviewer( m_scene->m_world );

					skip_component_editors = true;
				}
			}
		}

		if ( !skip_component_editors )
			ComponentReflectorTable::s_singleton.DoEditorUI( *m_scene->m_assetManager, m_scene->m_world, m_selectedEntity );
	}

	ImGui::End();

	if ( ImGui::Begin( std::format( "Entity Hierarchy: {}###{}", m_scene->m_path, (u64)(this + 2) ).c_str(), &m_open, ImGuiWindowFlags_MenuBar ) )
	{
		ImGui::SetWindowSize( { 320, 720 }, ImGuiCond_Once );

		bool open_add_entity_popup = false;
		bool open_import_prefab_popup = false;

		if ( ImGui::BeginMenuBar() )
		{
			if ( ImGui::BeginMenu( "Entities" ) )
			{
				if ( ImGui::Selectable( "Add Entity" ) )
					open_add_entity_popup = true;

				ImGui::EndMenu();
			}

			if ( ImGui::BeginMenu( "Prefabs" ) )
			{
				if ( ImGui::Selectable( "Import Scene" ) )
					open_import_prefab_popup = true;

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		if ( open_add_entity_popup )
			ImGui::OpenPopup( "Add Entity" );

		if ( open_import_prefab_popup )
			ImGui::OpenPopup( "Import Scene" );

		if ( ImGui::BeginPopup( "Add Entity" ) )
		{
			static std::string name;
			ImGui::InputText( "Name", &name );

			if ( ImGui::Button( "Cancel" ) )
				ImGui::CloseCurrentPopup();

			ImGui::SameLine();
			if ( ImGui::Button( "Ok" ) )
			{
				m_scene->m_world.AddEntity( Core::Name( name ) );

				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if ( ImGui::BeginPopup( "Import Scene" ) )
		{
			static std::string path;
			ImGui::InputText( "Path", &path );

			if ( ImGui::Button( "Cancel" ) )
				ImGui::CloseCurrentPopup();

			ImGui::SameLine();
			if ( ImGui::Button( "Ok" ) )
			{
				auto scene = m_scene->m_assetManager->Load< Scene >( path );

				if ( scene && LOG_ASSERT( scene->GetLoadingState() == LoadingState::Loaded ) )
				{
					const onyx::ecs::EntityID scene_instance_root = m_scene->m_world.AddEntity(
						onyx::Core::Name( std::format( "Scene Instance: {}", path ) )
					);

					m_scene->m_world.AddComponent( scene_instance_root, SceneInstance( scene, scene_instance_root ) );

					IDMap entity_id_map;
					scene->CopyToWorld( m_scene->m_world, entity_id_map );

					for ( auto& [src_entity, dst_entity] : entity_id_map )
						m_scene->m_world.AddComponent( dst_entity, SceneInstance( scene, scene_instance_root, src_entity ) );
				}

				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		std::vector< EntityID > stack;

		for ( const EntityID& root_entity : root_entities )
		{
			DoRecursiveEntityHierarchy( hierarchy, root_entity );
		}
	}

	ImGui::End();

	ImGui::End();
}

std::string SceneEditor::GetWindowTitle() const
{
	return std::format( "{}: {}###{}", s_name, !m_scene ? "" : m_scene->m_path, (u64)this );
}

void SceneEditor::DoRecursiveEntityHierarchy( const std::unordered_map< EntityID, std::vector< EntityID > >& hierarchy, EntityID entity )
{
	ImGuiScopedID scoped_id( entity );

	std::string name;
	if ( const Core::Name* const name_component = m_scene->m_world.GetComponent< Core::Name >( entity ) )
		name = std::format( "{}({})###{}", name_component->name, entity, entity );
	else
		name = std::format( "{}###{}", entity, entity );

	if ( ImGui::Button( "[-]" ) )
	{
		m_scene->m_world.RemoveEntity( entity );
		return;
	}

	ImGui::SameLine();
	if ( ImGui::Button( "Select" ) )
		m_selectedEntity = entity;

	ImGui::SameLine();
	if ( ImGui::CollapsingHeader( name.c_str() ) )
	{
		ImGuiScopedIndent scoped_indent( 20.f );

		if ( auto iter = hierarchy.find( entity ); iter != hierarchy.end() )
			for ( auto child : iter->second )
				DoRecursiveEntityHierarchy( hierarchy, child );

		if ( ImGui::Button( "[+]" ) )
			ImGui::OpenPopup( "Add Entity" );

		if ( ImGui::BeginPopup( "Add Entity" ) )
		{
			static std::string name;
			ImGui::InputText( "Name", &name );

			if ( ImGui::Button( "Cancel" ) )
				ImGui::CloseCurrentPopup();

			ImGui::SameLine();
			if ( ImGui::Button( "Ok" ) )
			{
				m_scene->m_world.AddEntity(
					Core::Name( name ),
					Core::AttachedTo( entity )
				);

				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
}

}
