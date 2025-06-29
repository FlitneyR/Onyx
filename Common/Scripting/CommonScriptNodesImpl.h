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

// AddContext
#define __SCRIPT_NODE_ADD_CONTEXT_GENERIC_BODY( cond, value ) \
	outputs.Ctx_Out = inputs.Ctx; \
	if ( !LOG_ASSERT( inputs.Ctx && inputs.Name cond ) ) \
		return Failed; \
	inputs.Ctx->AddInput( inputs.Name, value ); \
	return Then; \

SCRIPT_NODE_IMPL( ScriptNode_AddContext_AssetManager )
{ __SCRIPT_NODE_ADD_CONTEXT_GENERIC_BODY( && inputs.AssetManager, inputs.AssetManager ); }

SCRIPT_NODE_IMPL( ScriptNode_AddContext_Cmd )
{ __SCRIPT_NODE_ADD_CONTEXT_GENERIC_BODY( && inputs.Cmd, inputs.Cmd ); }

SCRIPT_NODE_IMPL( ScriptNode_AddContext_String )
{ __SCRIPT_NODE_ADD_CONTEXT_GENERIC_BODY( && inputs.String, &inputs.String ); }

SCRIPT_NODE_IMPL( ScriptNode_AddContext_Transform2D )
{ __SCRIPT_NODE_ADD_CONTEXT_GENERIC_BODY( && inputs.Transform2D, inputs.Transform2D ); }

SCRIPT_NODE_IMPL( ScriptNode_AddContext_Vec2 )
{ __SCRIPT_NODE_ADD_CONTEXT_GENERIC_BODY( , inputs.Vec2 ); }

// AddEntity
SCRIPT_NODE_IMPL( ScriptNode_AddEntity )
{
	onyx::ecs::CommandBuffer* cmd = inputs.Cmd;
	if ( !cmd )
		if ( onyx::ecs::CommandBuffer** _cmd = ctx.GetInput< onyx::ecs::CommandBuffer* >( "Cmd"_name ) )
			cmd = *_cmd;

	if ( outputs.Cmd_Out = cmd )
		outputs.Entity = cmd->AddEntity();

	return Then;
}

// BoolLiteral
SCRIPT_NODE_CUSTOM_LOAD( ScriptNode_BoolLiteral )
{
	reader.GetLiteral( "Value"_name, self.m_extras.value );
}

SCRIPT_NODE_CUSTOM_SAVE( ScriptNode_BoolLiteral )
{
	writer.SetLiteral( "Value"_name, self.m_extras.value );
}

SCRIPT_NODE_CUSTOM_UI( ScriptNode_BoolLiteral )
{
	ImGui::Checkbox( "Value", &self.m_extras.value );
}

SCRIPT_NODE_IMPL( ScriptNode_BoolLiteral )
{
	outputs.Value = extras.value;
	return Then;
}

// Branch
SCRIPT_NODE_IMPL( ScriptNode_Branch )
{
	outputs.PassThrough = inputs.Condition;
	return inputs.Condition ? True : False;
}

// CopyContext
SCRIPT_NODE_IMPL( ScriptNode_CopyContext )
{
	onyx::ScriptContext* context = inputs.Ctx;
	if ( !context )
		context = &ctx;

	outputs.Ctx_Out = context;
	outputs.Ctx_Copy = nullptr;

	if ( !LOG_ASSERT( context ) )
		return Failed;
	
	extras.ctx = *context;
	outputs.Ctx_Copy = &extras.ctx;
	return Then;
}

// FloatLiteral
SCRIPT_NODE_CUSTOM_UI( ScriptNode_FloatLiteral )
{
	ImGui::SetNextItemWidth( 100 );
	ImGui::InputFloat( "Value", &self.m_extras.Value );
	self.m_outputs.Value = self.m_extras.Value;
}

SCRIPT_NODE_CUSTOM_LOAD( ScriptNode_FloatLiteral )
{
	reader.GetLiteral( "Value"_name, self.m_extras.Value );
	self.m_outputs.Value = self.m_extras.Value;
}

SCRIPT_NODE_CUSTOM_SAVE( ScriptNode_FloatLiteral )
{
	writer.SetLiteral( "Value"_name, self.m_extras.Value );
}

SCRIPT_NODE_IMPL( ScriptNode_FloatLiteral )
{
	outputs.Value = extras.Value;
	return Then;
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

// MakeScriptContext
SCRIPT_NODE_IMPL( ScriptNode_MakeScriptContext )
{
	outputs.Ctx = &extras.ctx;
	return Then;
}

// MakeVec2
SCRIPT_NODE_CUSTOM_LOAD( ScriptNode_MakeVec2 )
{
	reader.GetLiteral( "X"_name, self.m_inputs.X );
	reader.GetLiteral( "Y"_name, self.m_inputs.Y );
}

SCRIPT_NODE_CUSTOM_SAVE( ScriptNode_MakeVec2 )
{
	writer.SetLiteral( "X"_name, self.m_inputs.X );
	writer.SetLiteral( "Y"_name, self.m_inputs.Y );
}

SCRIPT_NODE_IMPL( ScriptNode_MakeVec2 )
{
	outputs.Vec2 = { inputs.X, inputs.Y };
	return Then;
}

// RunScript
SCRIPT_NODE_IMPL( ScriptNode_RunScript )
{
	onyx::ScriptContext* context = inputs.Ctx;
	if ( !context )
		context = &ctx;

	onyx::AssetManager* asset_manager = inputs.AssetManager;
	if ( !asset_manager )
		if ( onyx::AssetManager** al = ctx.GetInput< onyx::AssetManager* >( "AssetManager"_name ) )
			asset_manager = *al;

	outputs.Ctx_Out = context;

	if ( !LOG_ASSERT( asset_manager && inputs.ScriptPath && context ) )
		return Failed;

	std::shared_ptr< Script > script = asset_manager->Load< Script >( inputs.ScriptPath );
	if ( !LOG_ASSERT( script ) )
		return Failed;

	outputs.Succeeded = onyx::ScriptRunner( script, *context ).Run();

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
