#pragma once
#include "Sprite.h"
#include "Common/Scripting/ScriptMacros.h"

// MakeSprite
#define MakeSprite_Inputs( f )						\
	f( onyx::AssetLoader*, AssetLoader, = nullptr )	\
	f( const char*, ImageAssetPath, = nullptr )		\

#define MakeSprite_Extras	\
	onyx::Sprite sprite;	\

#define MakeSprite_Outputs( f )				\
	f( onyx::Sprite*, Sprite, = nullptr )	\

#define ScriptNode_MakeSprite( f ) \
	f( MakeSprite, MakeSprite_Inputs, MakeSprite_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, MakeSprite_Extras )

// MakeSpriteAnimator
#define MakeSpriteAnimator_Inputs( f )				\
	f( onyx::AssetLoader*, AssetLoader, = nullptr )	\
	f( const char*, AssetPath, = nullptr )			\
	f( bool, StartPlaying, = true)					\

#define MakeSpriteAnimator_Extras	\
	onyx::SpriteAnimator animator;	\

#define MakeSpriteAnimator_Outputs( f )						\
	f( onyx::SpriteAnimator*, SpriteAnimator, = nullptr )	\

#define ScriptNode_MakeSpriteAnimator( f ) \
	f( MakeSpriteAnimator, MakeSpriteAnimator_Inputs, MakeSpriteAnimator_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, MakeSpriteAnimator_Extras )

// AddComponent_Sprite
#define AddComponent_Sprite_Inputs( f )						\
	f( onyx::ecs::CommandBuffer*, Cmd, = nullptr )			\
	f( onyx::ecs::EntityID, Entity, = onyx::ecs::NoEntity )	\
	f( onyx::Sprite*, Sprite, = nullptr )					\

#define AddComponent_Sprite_Outputs( f )						\
	f( onyx::ecs::CommandBuffer*, Cmd_Out, = nullptr )			\
	f( onyx::ecs::EntityID, Entity_Out, = onyx::ecs::NoEntity )	\

#define ScriptNode_AddComponent_Sprite( f ) \
	f( AddComponent_Sprite, AddComponent_Sprite_Inputs, AddComponent_Sprite_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

// AddComponent_SpriteAnimator
#define AddComponent_SpriteAnimator_Inputs( f )				\
	f( onyx::ecs::CommandBuffer*, Cmd, = nullptr )			\
	f( onyx::ecs::EntityID, Entity, = onyx::ecs::NoEntity )	\
	f( onyx::SpriteAnimator*, Animator, = nullptr )			\

#define AddComponent_SpriteAnimator_Outputs( f )				\
	f( onyx::ecs::CommandBuffer*, Cmd_Out, = nullptr )			\
	f( onyx::ecs::EntityID, Entity_Out, = onyx::ecs::NoEntity )	\

#define ScriptNode_AddComponent_SpriteAnimator( f ) \
	f( AddComponent_SpriteAnimator, AddComponent_SpriteAnimator_Inputs, AddComponent_SpriteAnimator_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

// AddComponent_ParallaxBackground
#define AddComponent_ParallaxBackground_Inputs( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd, = nullptr ) \
	f( onyx::ecs::EntityID, Entity, = onyx::ecs::NoEntity ) \
	f( glm::vec2, ScrollRate, = { 1.f, 1.f } ) \
	f( glm::vec2, Scale, = { 1.f, 1.f } ) \

#define AddComponent_ParallaxBackground_Outputs( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd_Out, = nullptr ) \
	f( onyx::ecs::EntityID, Entity_Out, = onyx::ecs::NoEntity ) \

#define ScriptNode_AddComponent_ParallaxBackground( f ) \
	f( AddComponent_ParallaxBackground, AddComponent_ParallaxBackground_Inputs, AddComponent_ParallaxBackground_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

#define ONYX_SPRITE_SCRIPT_NODES( f )				\
	f( ScriptNode_MakeSprite )						\
	f( ScriptNode_MakeSpriteAnimator )				\
	f( ScriptNode_AddComponent_Sprite )				\
	f( ScriptNode_AddComponent_SpriteAnimator )		\
	f( ScriptNode_AddComponent_ParallaxBackground )	\
