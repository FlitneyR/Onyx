#pragma once

#include "Player.h"
#include "Common/ECS/CommandBuffer.h"

namespace onyx
{

namespace ScriptNodes { Player_ScriptNodes( SCRIPT_NODE_DECL ) }

SCRIPT_NODE_IMPL( ScriptNode_AddComponent_PlayerController )
{
	onyx::ecs::CommandBuffer* cmd = inputs.Cmd;
	if ( !cmd )
		if ( onyx::ecs::CommandBuffer** _cmd = ctx.GetInput< onyx::ecs::CommandBuffer* >( "Cmd"_name ) )
			cmd = *_cmd;

	onyx::AssetManager* asset_manager = inputs.AssetManager;
	if ( !asset_manager )
		if ( onyx::AssetManager** _asset_manager = ctx.GetInput< onyx::AssetManager* >( "AssetManager"_name ) )
			asset_manager = *_asset_manager;

	outputs.Cmd_Out = cmd;
	outputs.Entity_Out = inputs.Entity;

	if ( !LOG_ASSERT( cmd && inputs.Entity ) )
		return Failed;

	cmd->AddComponent( inputs.Entity, chrono::PlayerController {
		inputs.MoveSpeed, inputs.TurnSpeed,
		!asset_manager ? nullptr : asset_manager->Load< TextureAnimationAsset >( inputs.EngineIdleAnimationPath ),
		!asset_manager ? nullptr : asset_manager->Load< TextureAnimationAsset >( inputs.EngineBoostAnimationPath ),
		inputs.EngineEffectEntity
	} );

	return Then;
}

}
