#include "Transform.h"

namespace onyx
{

COMPONENT_REFLECTOR( Transform2D )
{
	DECLARE_COMPONENT_REFLECTOR_SINGLETON();

	DESERIALISE_COMPONENT()
	{
		Transform2D* _comp = world.GetComponent< Transform2D >( entity );
		Transform2D& comp = _comp ? *_comp : world.AddComponent< Transform2D >( entity, {} );

		reader.GetLiteral( "Position"_name, comp.m_position );
		reader.GetLiteral( "Scale"_name, comp.m_scale );
		reader.GetLiteral( "Rotation"_name, comp.m_rotation );
	}

	SERIALISE_COMPONENT()
	{
		if ( const Transform2D* const comp = entity.Get< Transform2D >() )
		{
			writer.AddChild( "Transform2D"_name )
				.SetLiteral( "Position"_name, comp->m_position )
				.SetLiteral( "Scale"_name, comp->m_scale )
				.SetLiteral( "Rotation"_name, comp->m_rotation );
		}
	}

	DO_COMPONENT_EDITOR_UI()
	{
		if ( Transform2D* const comp = world.GetComponent< Transform2D >( entity ) )
		{
			auto position = comp->m_position;
			auto scale = comp->m_scale;
			auto rotation = comp->m_rotation;

			bool any_edits = false;
			any_edits |= ImGui::InputFloat2( "Position", &position.x );
			any_edits |= ImGui::InputFloat2( "Scale", &scale.x );
			any_edits |= ImGui::InputFloat( "Rotation", &rotation );

			if ( any_edits )
			{
				comp->SetLocalPosition( position );
				comp->SetLocalScale( scale );
				comp->SetLocalRotation( rotation );
			}
		}
	}

	SERIALISE_COMPONENT_DIFF()
	{
		const Transform2D* const comp = world.GetComponent< Transform2D >( entity );
		const Transform2D* const src_comp = src_world.GetComponent< Transform2D >( src_entity );
		BjSON::IReadWriteObject* w = nullptr;

		if ( !src_comp || src_comp->m_position != comp->m_position )
		{
			if ( !w ) w = &writer.AddChild( "Transform2D"_name );
			w->SetLiteral( "Position"_name, comp->m_position );
		}

		if ( !src_comp || src_comp->m_scale != comp->m_scale )
		{
			if ( !w ) w = &writer.AddChild( "Transform2D"_name );
			w->SetLiteral( "Scale"_name, comp->m_scale );
		}

		if ( !src_comp || src_comp->m_rotation != comp->m_rotation )
		{
			if ( !w ) w = &writer.AddChild( "Transform2D"_name );
			w->SetLiteral( "Rotation"_name, comp->m_rotation );
		}
	}
};

REGISTER_COMPONENT_REFLECTOR( Transform2D );

}
