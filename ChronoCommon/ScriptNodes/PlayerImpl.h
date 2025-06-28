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

	onyx::AssetLoader* asset_loader = inputs.AssetLoader;
	if ( !asset_loader )
		if ( onyx::AssetLoader** _asset_loader = ctx.GetInput< onyx::AssetLoader* >( "AssetLoader"_name ) )
			asset_loader = *_asset_loader;

	outputs.Cmd_Out = cmd;
	outputs.Entity_Out = inputs.Entity;

	if ( !LOG_ASSERT( cmd && inputs.Entity ) )
		return Failed;

	cmd->AddComponent( inputs.Entity, chrono::PlayerController {
		inputs.MoveSpeed, inputs.TurnSpeed,
		!asset_loader ? nullptr : asset_loader->Load< TextureAnimationAsset >( inputs.EngineIdleAnimationPath ),
		!asset_loader ? nullptr : asset_loader->Load< TextureAnimationAsset >( inputs.EngineBoostAnimationPath ),
		inputs.EngineEffectEntity
	} );

	return Then;
}

}
