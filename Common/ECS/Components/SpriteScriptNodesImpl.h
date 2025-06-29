#pragma once
#include "SpriteScriptNodes.h"
#include "Common/Scripting/ScriptMacros.h"

#include "Common/ECS/CommandBuffer.h"

#include "Common/Graphics/Texture.h"

namespace onyx
{

namespace ScriptNodes { ONYX_SPRITE_SCRIPT_NODES( SCRIPT_NODE_DECL ) }

SCRIPT_NODE_IMPL( ScriptNode_MakeSprite )
{
	outputs.Sprite = nullptr;

	onyx::AssetManager* asset_manager = inputs.AssetManager;
	if ( !asset_manager )
		if ( onyx::AssetManager** al = ctx.GetInput< onyx::AssetManager* >( "AssetManager"_name ) )
			asset_manager = *al;

	if ( !LOG_ASSERT( asset_manager && inputs.ImageAssetPath ) )
		return Failed;

	std::shared_ptr< TextureAsset > texture = asset_manager->Load< TextureAsset >( inputs.ImageAssetPath );
	if ( !LOG_ASSERT( texture ) )
		return Failed;

	extras.sprite.m_texture = texture->GetGraphicsResource();
	outputs.Sprite = &extras.sprite;

	return Then;
}

SCRIPT_NODE_IMPL( ScriptNode_MakeSpriteAnimator )
{
	outputs.SpriteAnimator = nullptr;

	onyx::AssetManager* asset_manager = inputs.AssetManager;
	if ( !asset_manager )
		if ( onyx::AssetManager** al = ctx.GetInput< onyx::AssetManager* >( "AssetManager"_name ) )
			asset_manager = *al;

	if ( !LOG_ASSERT( asset_manager && inputs.AssetPath ) )
		return Failed;

	std::shared_ptr< TextureAnimationAsset > animation = asset_manager->Load< TextureAnimationAsset >( inputs.AssetPath );
	if ( !LOG_ASSERT( animation ) )
		return Failed;

	extras.animator.m_animation = animation;
	extras.animator.m_playRate = inputs.StartPlaying ? animation->m_rate : 0.f;
	outputs.SpriteAnimator = &extras.animator;

	return Then;
}

SCRIPT_NODE_IMPL( ScriptNode_AddComponent_Sprite )
{
	onyx::ecs::CommandBuffer* cmd = inputs.Cmd;
	if ( !cmd )
		if ( onyx::ecs::CommandBuffer** _cmd = ctx.GetInput< onyx::ecs::CommandBuffer* >( "Cmd"_name ) )
			cmd = *_cmd;

	outputs.Cmd_Out = cmd;
	outputs.Entity_Out = inputs.Entity;

	if ( !LOG_ASSERT( cmd && inputs.Entity ) )
		return Failed;

	cmd->AddComponent( inputs.Entity, inputs.Sprite ? *inputs.Sprite : Sprite() );
	return Then;
}

SCRIPT_NODE_IMPL( ScriptNode_AddComponent_SpriteAnimator )
{
	onyx::ecs::CommandBuffer* cmd = inputs.Cmd;
	if ( !cmd )
		if ( onyx::ecs::CommandBuffer** _cmd = ctx.GetInput< onyx::ecs::CommandBuffer* >( "Cmd"_name ) )
			cmd = *_cmd;

	outputs.Cmd_Out = cmd;
	outputs.Entity_Out = inputs.Entity;

	if ( !LOG_ASSERT( cmd && inputs.Entity ) )
		return Failed;

	cmd->AddComponent( inputs.Entity, inputs.Animator ? *inputs.Animator : SpriteAnimator() );
	return Then;
}

SCRIPT_NODE_IMPL( ScriptNode_AddComponent_ParallaxBackground )
{
	onyx::ecs::CommandBuffer* cmd = inputs.Cmd;
	if ( !cmd )
		if ( onyx::ecs::CommandBuffer** _cmd = ctx.GetInput< onyx::ecs::CommandBuffer* >( "Cmd"_name ) )
			cmd = *_cmd;

	outputs.Cmd_Out = cmd;
	outputs.Entity_Out = inputs.Entity;

	if ( !LOG_ASSERT( cmd && inputs.Entity ) )
		return Failed;

	cmd->AddComponent( inputs.Entity, ParallaxBackground { inputs.ScrollRate, inputs.Scale } );

	return Then;
}

}
