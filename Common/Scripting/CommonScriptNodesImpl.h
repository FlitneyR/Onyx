#include "CommonScriptNodes.h"

#include "Common/ECS/CommandBuffer.h"
#include "Common/Scripting/Script.h"

#include "imgui_stdlib.h"

namespace onyx
{

namespace ScriptNodes
{

ONYX_COMMON_SCRIPT_NODES( SCRIPT_NODE_DECL )

}

// AddEntity
SCRIPT_NODE_IMPL( ScriptNode_AddEntity )
{
	if ( outputs.Cmd_Out = inputs.Cmd )
		outputs.Entity = inputs.Cmd->AddEntity();

	return Then;
}

// Branch
SCRIPT_NODE_IMPL( ScriptNode_Branch )
{
	outputs.PassThrough = inputs.Condition;
	return inputs.Condition ? True : False;
}

// IntLiteral
SCRIPT_NODE_CUSTOM_UI( ScriptNode_IntLiteral )
{
	ImGui::SetNextItemWidth( 100 );
	ImGui::InputInt( "Value", &self.m_extras.Value );
	self.m_outputs.Value = self.m_extras.Value;
}

SCRIPT_NODE_CUSTOM_LOAD( ScriptNode_IntLiteral )
{
	reader.GetLiteral( "Value"_name, self.m_extras.Value );
	self.m_outputs.Value = self.m_extras.Value;
}

SCRIPT_NODE_CUSTOM_SAVE( ScriptNode_IntLiteral )
{
	writer.SetLiteral( "Value"_name, self.m_extras.Value );
}

SCRIPT_NODE_IMPL( ScriptNode_IntLiteral )
{
	outputs.Value = extras.Value;
	return Then;
}

// IsValidEntity
SCRIPT_NODE_IMPL( ScriptNode_IsValidEntity )
{
	return ( outputs.IsValid = inputs.Entity != onyx::ecs::NoEntity ) ? Valid : NotValid;
}

// Log_Message
SCRIPT_NODE_IMPL( ScriptNode_LogInfoMessage )
{
	INFO( "{}", inputs.Message );
	return Then;
}

SCRIPT_NODE_IMPL( ScriptNode_LogWarningMessage )
{
	WARN( "{}", inputs.Message );
	return Then;
}

SCRIPT_NODE_IMPL( ScriptNode_LogErrorMessage )
{
	ERROR( "{}", inputs.Message );
	return Then;
}

// StringLiteral
SCRIPT_NODE_CUSTOM_UI( ScriptNode_StringLiteral )
{
	ImGui::SetNextItemWidth( 100 );
	ImGui::InputText( "String", &self.m_extras.String );
	self.m_outputs.String = self.m_extras.String.c_str();
}

SCRIPT_NODE_CUSTOM_LOAD( ScriptNode_StringLiteral )
{
	reader.GetLiteral( "String"_name, self.m_extras.String );
	self.m_outputs.String = self.m_extras.String.c_str();
}

SCRIPT_NODE_CUSTOM_SAVE( ScriptNode_StringLiteral )
{
	writer.SetLiteral( "String"_name, self.m_extras.String );
}

SCRIPT_NODE_IMPL( ScriptNode_StringLiteral )
{
	outputs.String = extras.String.c_str();
	return Then;
}

}
