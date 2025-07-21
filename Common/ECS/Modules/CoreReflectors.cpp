#include "Common/ECS/ComponentReflector.h"
#include "Common/ECS/Modules/Core.h"
#include "Common/ECS/Scene.h"

#include "imgui_stdlib.h"

using AttachedTo = onyx::Core::AttachedTo;
using Name = onyx::Core::Name;
using Transform2D = onyx::Core::Transform2D;

COMPONENT_REFLECTOR( AttachedTo )
{
	COMPONENT_REFLECTOR_HEADER( AttachedTo );

	#define xproperties( f )\
		f( AttachedTo, EntityID, localeEntity, "Parent" )\

	DEFAULT_SERIALISE_COMPONENT( AttachedTo, xproperties );
	DEFAULT_DESERIALISE_COMPONENT( AttachedTo, xproperties );
	DEFAULT_SERIALISE_COMPONENT_EDITS( AttachedTo, xproperties );

	DO_COMPONENT_EDITOR_UI()
	{
		BEGIN_COMPONENT_EDITOR_UI( AttachedTo, attachment );

		if ( const Name* parent_name = world.GetComponent< Name >( attachment.localeEntity ) )
			ImGui::Text( "%s(%d)", parent_name->name.c_str(), attachment.localeEntity );
		else
			ImGui::Text( "%d", attachment.localeEntity );
	}

	POST_COPY_TO_WORLD()
	{
		BEGIN_POST_COPY_TO_WORLD( AttachedTo, attachment );
		UpdateEntityID( attachment.localeEntity, entity_id_map );
	}

	#undef xproperties
};

COMPONENT_REFLECTOR( Name )
{
	COMPONENT_REFLECTOR_HEADER( Name );

	#define xproperties( f )\
		f( Name, std::string, name, "Name" )\

	DEFAULT_REFLECTOR( Name, xproperties );

	#undef xproperties
};

COMPONENT_REFLECTOR( Transform2D )
{
	COMPONENT_REFLECTOR_HEADER( Transform2D );

	#define xproperties( f )\
		f( Transform2D, glm::vec2, m_position, "Position" )\
		f( Transform2D, glm::vec2, m_scale, "Scale" )\
		f( Transform2D, f32, m_rotation, "Rotation" )\

	DEFAULT_SERIALISE_COMPONENT( Transform2D, xproperties );
	DEFAULT_SERIALISE_COMPONENT_EDITS( Transform2D, xproperties );

	DESERIALISE_COMPONENT()
	{
		BEGIN_DESERIALISE_COMPONENT( Transform2D, component );
		xproperties( DEFAULT_DESERIALISE_PROPERTY );

		component.Refresh();
	}

	DO_COMPONENT_EDITOR_UI()
	{
		BEGIN_COMPONENT_EDITOR_UI( Transform2D, component );
		xproperties( DEFAULT_PROPERTY_EDITOR_UI );

		if ( __any_edits )
			component.Refresh();
	}

	#undef xproperties
};

DEFINE_COMPONENT_REFLECTOR( AttachedTo );
DEFINE_COMPONENT_REFLECTOR( Name );
DEFINE_COMPONENT_REFLECTOR( Transform2D );

void onyx::Core::RegisterReflectors( onyx::ecs::ComponentReflectorTable& table )
{
	REGISTER_COMPONENT_REFLECTOR( table, AttachedTo );
	REGISTER_COMPONENT_REFLECTOR( table, Name );
	REGISTER_COMPONENT_REFLECTOR( table, Transform2D );
}
