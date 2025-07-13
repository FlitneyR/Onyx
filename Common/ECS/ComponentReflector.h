#pragma once

#include "Common/Assets.h"
#include "Common/BjSON/BjSON.h"
#include "Common/ECS/World.h"
#include "Common/Utils.h"

namespace onyx::ecs
{

// source world id, destination world id
using IDMap = std::vector< std::pair< EntityID, EntityID > >;

struct IComponentReflector
{
	IComponentReflector( const char* const name, size_t type_hash )
		: m_name( name )
		, m_nameHash( BjSON::HashName( name ) )
		, m_typeHash( type_hash )
	{}
	
	const char* const m_name;
	const BjSON::NameHash m_nameHash;
	const size_t m_typeHash;

	virtual void DeserialiseComponent( const BjSON::IReadOnlyObject& reader, AssetManager& asset_manager, World& world, EntityID entity ) const = 0;
	virtual void SerialiseComponent( BjSON::IReadWriteObject& writer, World::EntityIterator& entity ) const = 0;
	virtual void DoEditorUI( AssetManager& asset_manager, World& world, EntityID entity ) const = 0;
	virtual void SerialiseEdits( BjSON::IReadWriteObject& writer, const std::set< BjSON::NameHash >& edits, World& world, EntityID entity ) const = 0;
	virtual void DoAddComponentButton( World& world, EntityID entity, const std::string& search_term ) const = 0;
	virtual void PostCopy( World& world, EntityID entity, const IDMap& entity_id_map ) const {}

	void UpdateEntityID( EntityID& entity, const IDMap& entity_id_map ) const
	{
		if ( auto iter = std::find_if( entity_id_map.begin(), entity_id_map.end(), [ entity ]( const std::pair< EntityID, EntityID >& pair )
			{ return pair.first == entity; } ); WEAK_ASSERT( iter != entity_id_map.end(), "Couldn't find an entity ID mapped to {}", entity ) )
			entity = iter->second;
	}

	template< typename T > static void DefaultSerialiseProperty( BjSON::IReadWriteObject& writer, const T& value, BjSON::NameHash name );
	template< typename T > static void DefaultDeserialiseProperty( const BjSON::IReadOnlyObject& reader, AssetManager& asset_manager, T& value, BjSON::NameHash name );
	// returns true if the property has been edited
	template< typename T > static bool DefaultPropertyEditorUI( World& world, AssetManager& asset_manager, T& value, const char* name );
	template< typename T > static void DefaultPropertyDiffHint( World& src_world, EntityID src_entity, const T& value );
};

#define DEFINE_DEFAULT_SERIALISE_PROPERTY( TypeName ) template<> void onyx::ecs::IComponentReflector::DefaultSerialiseProperty< TypeName >( BjSON::IReadWriteObject& writer, const TypeName& value, BjSON::NameHash name )
#define DEFINE_DEFAULT_DESERIALISE_PROPERTY( TypeName ) template<> void onyx::ecs::IComponentReflector::DefaultDeserialiseProperty< TypeName >( const BjSON::IReadOnlyObject& reader, AssetManager& asset_manager, TypeName& value, BjSON::NameHash name )
#define DEFINE_DEFAULT_PROPERTY_EDITOR_UI( TypeName ) template<> bool onyx::ecs::IComponentReflector::DefaultPropertyEditorUI< TypeName >( World& world, AssetManager& asset_manager, TypeName& value, const char* name )
#define DEFINE_DEFAULT_PROPERTY_DIFF_HINT( TypeName ) template<> void onyx::ecs::IComponentReflector::DefaultPropertyDiffHint< TypeName >( World& src_world, EntityID src_entity, const TypeName& value )

template< typename Component >
struct ComponentReflector;

struct ComponentReflectorTable
{
	static ComponentReflectorTable s_singleton;

	const IComponentReflector* GetReflector( size_t component_type_hash ) const;
	const IComponentReflector* GetReflector( BjSON::NameHash component_name_hash ) const;

	template< typename Component >
	void RegisterReflector()
	{
		RegisterReflector(
			onyx::ecs::ComponentReflector< Component >::s_singleton,
			onyx::ecs::ComponentReflector< Component >::s_singleton.m_nameHash,
			onyx::ecs::ComponentReflector< Component >::s_singleton.m_typeHash
		);
	}

	void RegisterReflector( const IComponentReflector& reflector, BjSON::NameHash component_name_hash, size_t component_type_hash );

	void SerialiseEdits( BjSON::IReadWriteObject& writer, const std::map< BjSON::NameHash, std::set< BjSON::NameHash > >& edits, World& world, EntityID entity );
	void DoEditorUI( AssetManager& asset_manager, World& world, EntityID entity );
	void PostCopyToWorld( World& world, EntityID entity, const IDMap& entity_id_map );

private:
	ComponentReflectorTable() = default;

	std::vector< const IComponentReflector* > m_reflectors;
	std::unordered_map< size_t, const IComponentReflector* > m_typeHashLookup;
	std::unordered_map< BjSON::NameHash, const IComponentReflector* > m_typeNameLookup;
};

#define COMPONENT_REFLECTOR_FRIEND( Component )	friend struct onyx::ecs::ComponentReflector< Component >
#define COMPONENT_REFLECTOR( Component )		template<> struct onyx::ecs::ComponentReflector< Component > : onyx::ecs::IComponentReflector
#define DESERIALISE_COMPONENT()					void DeserialiseComponent( const BjSON::IReadOnlyObject& reader, AssetManager& asset_manager, World& world, EntityID entity ) const override
#define SERIALISE_COMPONENT()					void SerialiseComponent( BjSON::IReadWriteObject& __writer, World::EntityIterator& entity ) const override
#define DO_COMPONENT_EDITOR_UI()				void DoEditorUI( AssetManager& asset_manager, World& world, EntityID entity ) const override
#define SERIALISE_COMPONENT_EDITS()				void SerialiseEdits( BjSON::IReadWriteObject& __writer, const std::set< BjSON::NameHash >& edits, World& world, EntityID entity ) const override
#define POST_COPY_TO_WORLD()					void PostCopy( World& world, EntityID entity, const IDMap& entity_id_map ) const override

#define COMPONENT_REFLECTOR_HEADER( Component )\
	void DoAddComponentButton( World& world, EntityID entity, const std::string& search_term ) const override\
	{\
		if ( !world.GetComponent< Component >( entity ) && ( search_term.empty() || strstr( #Component, search_term.c_str() ) ) && ImGui::Selectable( #Component ) )\
			world.AddComponent( entity, Component() );\
	}\
	ComponentReflector() : IComponentReflector( #Component, typeid( Component ).hash_code() ) {}\
	static ComponentReflector s_singleton

#define DEFINE_COMPONENT_REFLECTOR( Component )\
	onyx::ecs::ComponentReflector< Component > onyx::ecs::ComponentReflector< Component >::s_singleton{}

#define REGISTER_COMPONENT_REFLECTOR( table, Component )\
	table.RegisterReflector< Component >()

#define BEGIN_DESERIALISE_COMPONENT( Component, component )\
	Component* __##component = world.GetComponent< Component >( entity );\
	Component& component = __##component ? *__##component : world.AddComponent< Component >( entity, {} )\

#define BEGIN_SERIALISE_COMPONENT( Component, component )\
	const Component* __##component = entity.Get< Component >();\
	if ( !__##component ) return;\
	const Component& component = *__##component;\
	BjSON::IReadWriteObject& writer = __writer.AddChild( #Component##_name )\

#define BEGIN_COMPONENT_EDITOR_UI( Component, component )\
	ImGuiScopedID __scopedId( #Component );\
	Component* const __##component = world.GetComponent< Component >( entity );\
	if ( !__##component ) return;\
	Component& component = *__##component;\
	const Component* src_##component = nullptr;\
	World* src_world = nullptr;\
	EntityID src_entity = NoEntity;\
	std::set< BjSON::NameHash >* edits = nullptr;\
	bool __any_edits = false;\
	if ( SceneInstance* const scene_instance = world.GetComponent< SceneInstance >( entity ) )\
	{\
		if ( src_##component = ( src_world = &scene_instance->m_scene->m_world )->GetComponent< Component >( src_entity = scene_instance->m_sceneEntityId ) )\
			edits = &scene_instance->m_edits.insert( { #Component##_name, {} } ).first->second;\
	}\
	if ( src_##component )\
	{\
		if ( edits && !edits->empty() )\
		{\
			if ( ImGui::Button( "Revert" ) )\
			{\
				edits->clear();\
				component = *src_##component;\
				__any_edits = true;\
			}\
			ImGui::SameLine();\
		}\
	}\
	else\
	{\
		if ( ImGui::Button( "[-]" ) )\
			world.RemoveComponent< Component >( entity );\
		ImGui::SameLine();\
	}\
	if ( !ImGui::CollapsingHeader( #Component ) )\
	{\
		if ( src_##component && ImGui::IsItemHovered() )\
			ImGui::SetTooltip( "This component is part of a prefab instance, and cannot be removed" );\
		return;\
	}\
	if ( src_##component && ImGui::IsItemHovered() )\
		ImGui::SetTooltip( "This component is part of a prefab instance, and cannot be removed" );\
	ImGuiScopedID __imgui_scoped_id( entity );\
	ImGuiScopedIndent __imgui_scoped_indent( 10.f );\

#define BEGIN_SERIALISE_EDITS( Component, component )\
	const Component* const __##component = world.GetComponent< Component >( entity );\
	if ( !__##component ) return;\
	const Component& component = *__##component;\
	BjSON::IReadWriteObject* writer = nullptr
	// const Component* const src_##component = src_world.GetComponent< Component >( src_entity );\

#define BEGIN_POST_COPY_TO_WORLD( Component, component )\
	Component* const __##component = world.GetComponent< Component >( entity );\
	if ( !__##component ) return;\
	Component& component = *__##component;

#define IF_EDITED( Component, property_name )\
	if ( edits.find( property_name##_name ) != edits.end() && ( writer || ( writer = &__writer.AddChild( #Component##_name ) ) ) )

#define DIFF_HINT( component, ... ) if ( src_##component && ImGui::IsItemHovered() ) ImGui::SetTooltip( __VA_ARGS__ )

#define REGISTER_PROPERTY_CHANGED( name )\
	if ( edits ) edits->insert( name##_name )

#define DEFAULT_SERIALISE_PROPERTY( Component, PropertyType, property_name, name ) DefaultSerialiseProperty< PropertyType >( writer, component.property_name, name##_name );
#define DEFAULT_DESERIALISE_PROPERTY( Component, PropertyType, property_name, name ) DefaultDeserialiseProperty< PropertyType >( reader, asset_manager, component.property_name, name##_name );
#define DEFAULT_PROPERTY_EDITOR_UI( Component, PropertyType, property_name, name ) \
	if ( src_component && edits && edits->find( name##_name ) != edits->end() )\
	{\
		ImGuiScopedID __scopedId( name );\
		if( ImGui::Button( "Revert" ) )\
		{\
			component.property_name = src_component->property_name; \
			edits->erase( name##_name ); \
			__any_edits = true;\
		}\
		if ( ImGui::IsItemHovered() )\
			DefaultPropertyDiffHint< PropertyType >( *src_world, src_entity, src_component->property_name );\
		ImGui::SameLine();\
	}\
	if ( DefaultPropertyEditorUI< PropertyType >( world, asset_manager, component.property_name, name ) )\
	{\
		__any_edits = true;\
		REGISTER_PROPERTY_CHANGED( name );\
	}\

#define DEFAULT_SERIALISE_PROPERTY_EDIT( Component, PropertyType, property_name, name ) \
	IF_EDITED( Component, name )\
		DefaultSerialiseProperty< PropertyType >( *writer, component.property_name, name##_name );

#define DEFAULT_SERIALISE_COMPONENT( Component, xproperties )\
	SERIALISE_COMPONENT() { BEGIN_SERIALISE_COMPONENT( Component, component ); xproperties( DEFAULT_SERIALISE_PROPERTY ) }

#define DEFAULT_DESERIALISE_COMPONENT( Component, xproperties )\
	DESERIALISE_COMPONENT() { BEGIN_DESERIALISE_COMPONENT( Component, component ); xproperties( DEFAULT_DESERIALISE_PROPERTY ) }

#define DEFAULT_SERIALISE_COMPONENT_EDITS( Component, xproperties )\
	SERIALISE_COMPONENT_EDITS() { BEGIN_SERIALISE_EDITS( Component, component ); xproperties( DEFAULT_SERIALISE_PROPERTY_EDIT ) }

#define DEFAULT_COMPONENT_EDITOR_UI( Component, xproperties )\
	DO_COMPONENT_EDITOR_UI() { BEGIN_COMPONENT_EDITOR_UI( Component, component ); xproperties( DEFAULT_PROPERTY_EDITOR_UI ) }

#define DEFAULT_REFLECTOR( Component, xproperties )\
	DEFAULT_SERIALISE_COMPONENT( Component, xproperties );\
	DEFAULT_DESERIALISE_COMPONENT( Component, xproperties );\
	DEFAULT_SERIALISE_COMPONENT_EDITS( Component, xproperties );\
	DEFAULT_COMPONENT_EDITOR_UI( Component, xproperties )\

//#define DEFAULT_DESERIALISE_PROPERTY( MemberName, member ) DefaultDeserialiseProperty( reader, comp.member );
//#define DEFAULT_DESERIALISE( Component, xproperties ) \
//	DESERIALISE_COMPONENT()\
//	{\
//		Component* _comp = world.GetComponent< Component >( entity );\
//		Component& comp = _comp ? *_comp : world.AddComponent< Transform2D >( entity, {} );\
//		xproperties( DEFAULT_DESERIALISE_PROPERTY )\
//	}\

}
