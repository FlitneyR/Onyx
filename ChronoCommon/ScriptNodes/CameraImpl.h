#pragma once
#include "Camera.h"
#include "ChronoCommon/Components/Camera.h"

namespace onyx
{

namespace ScriptNodes { Camera_ScriptNodes( SCRIPT_NODE_DECL ) }

SCRIPT_NODE_IMPL( ScriptNode_AddComponent_Camera )
{
	onyx::ecs::CommandBuffer* cmd = inputs.Cmd;
	if ( !cmd )
		if ( onyx::ecs::CommandBuffer** _cmd = ctx.GetInput< onyx::ecs::CommandBuffer* >( "Cmd"_name ) )
			cmd = *_cmd;

	outputs.Cmd_Out = cmd;
	outputs.Entity_Out = inputs.Entity;

	if ( !LOG_ASSERT( cmd && inputs.Entity ) )
		return Failed;

	cmd->AddComponent( inputs.Entity, chrono::Camera {
		inputs.MinFov,
		inputs.MaxFov,
		inputs.Margin,
		inputs.MoveSpeed,
		inputs.ZoomSpeed
	} );

	return Then;
}

SCRIPT_NODE_IMPL( ScriptNode_AddComponent_CameraFocus )
{
	onyx::ecs::CommandBuffer* cmd = inputs.Cmd;
	if ( !cmd )
		if ( onyx::ecs::CommandBuffer** _cmd = ctx.GetInput< onyx::ecs::CommandBuffer* >( "Cmd"_name ) )
			cmd = *_cmd;

	outputs.Cmd_Out = cmd;
	outputs.Entity_Out = inputs.Entity;

	if ( !LOG_ASSERT( cmd && inputs.Entity ) )
		return Failed;

	cmd->AddComponent( inputs.Entity, chrono::CameraFocus() );

	return Then;
}

}
