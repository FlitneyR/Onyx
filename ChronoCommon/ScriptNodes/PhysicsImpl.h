#pragma once

#include "Physics.h"
#include "Common/ECS/CommandBuffer.h"

namespace onyx
{

namespace ScriptNodes { Physics_ScriptNodes( SCRIPT_NODE_DECL ) }

SCRIPT_NODE_IMPL( ScriptNode_AddComponent_PhysicsBody )
{
	onyx::ecs::CommandBuffer* cmd = inputs.Cmd;
	if ( !cmd )
		if ( onyx::ecs::CommandBuffer** _cmd = ctx.GetInput< onyx::ecs::CommandBuffer* >( "Cmd"_name ) )
			cmd = *_cmd;

	outputs.Cmd_Out = cmd;
	outputs.Entity_Out = inputs.Entity;

	if ( !( cmd && inputs.Entity ) )
	{
		WARN( "ScriptNode_AddComponent_PhysicsBody Failed" );
		return Failed;
	}

	cmd->AddComponent( inputs.Entity, chrono::PhysicsBody { inputs.LinearFriction, inputs.AngularFriction } );

	return Then;
}

SCRIPT_NODE_IMPL( ScriptNode_AddComponent_Collider )
{
	onyx::ecs::CommandBuffer* cmd = inputs.Cmd;
	if ( !cmd )
		if ( onyx::ecs::CommandBuffer** _cmd = ctx.GetInput< onyx::ecs::CommandBuffer* >( "Cmd"_name ) )
			cmd = *_cmd;

	outputs.Cmd_Out = cmd;
	outputs.Entity_Out = inputs.Entity;

	if ( !( cmd && inputs.Entity ) )
	{
		WARN( "ScriptNode_AddComponent_Collider Failed" );
		return Failed;
	}

	cmd->AddComponent( inputs.Entity, chrono::Collider { inputs.Radius } );

	return Then;
}

}
