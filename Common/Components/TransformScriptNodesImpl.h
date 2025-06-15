#include "Transform.h"
#include "Common/ECS/World.h"
#include "Common/ECS/CommandBuffer.h"

namespace onyx
{

namespace ScriptNodes
{

ONYX_TRANSFORM2D_SCRIPT_NODES( SCRIPT_NODE_DECL )

}


SCRIPT_NODE_CUSTOM_UI( ScriptNode_MakeTransform2D )
{
	ImGui::SetNextItemWidth( 100.f );
	ImGui::InputFloat2( "Default Position", (f32*)&self.m_inputs.Position );

	ImGui::SetNextItemWidth( 100.f );
	ImGui::InputFloat2( "Default Scale", (f32*)&self.m_inputs.Scale );

	ImGui::SetNextItemWidth( 100.f );
	ImGui::InputFloat( "Default Rotation", (f32*)&self.m_inputs.Rotation );
}

SCRIPT_NODE_CUSTOM_LOAD( ScriptNode_MakeTransform2D )
{
	reader.GetLiteral( "Position"_name, self.m_inputs.Position );
	reader.GetLiteral( "Scale"_name, self.m_inputs.Scale );
	reader.GetLiteral( "Rotation"_name, self.m_inputs.Rotation );
}

SCRIPT_NODE_CUSTOM_SAVE( ScriptNode_MakeTransform2D )
{
	writer.SetLiteral( "Position"_name, self.m_inputs.Position );
	writer.SetLiteral( "Scale"_name, self.m_inputs.Scale );
	writer.SetLiteral( "Rotation"_name, self.m_inputs.Rotation );
}

SCRIPT_NODE_IMPL( ScriptNode_MakeTransform2D )
{
	extras.Transform.SetLocale( inputs.Locale );
	extras.Transform.SetPosition( inputs.Position );
	extras.Transform.SetRotation( inputs.Rotation );
	extras.Transform.SetScale( inputs.Scale );

	outputs.Transform2D = &extras.Transform;

	return Then;
}

SCRIPT_NODE_IMPL( ScriptNode_AddComponent_Transform2D )
{
	if ( outputs.Cmd_Out = inputs.Cmd )
		inputs.Cmd->AddComponent( outputs.Entity_Out = inputs.Entity, inputs.Transform2D );

	return Failed;
}

}
