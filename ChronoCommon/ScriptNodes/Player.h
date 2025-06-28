#pragma once

#include "ChronoCommon/Components/Player.h"
#include "Common/Scripting/ScriptMacros.h"

// AddComponent_PlayerController
#define AddComponent_PlayerController_Inputs( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd, = nullptr ) \
	f( onyx::ecs::EntityID, Entity, = onyx::ecs::NoEntity ) \
	f( onyx::AssetLoader*, AssetLoader, = nullptr ) \
	f( f32, TurnSpeed, = 1.f ) \
	f( f32, MoveSpeed, = 1.f ) \
	f( const char*, EngineIdleAnimationPath, = "" ) \
	f( const char*, EngineBoostAnimationPath, = "" ) \
	f( onyx::ecs::EntityID, EngineEffectEntity, = onyx::ecs::NoEntity ) \

#define AddComponent_PlayerController_Outputs( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd_Out, = nullptr ) \
	f( onyx::ecs::EntityID, Entity_Out, = onyx::ecs::NoEntity ) \

#define ScriptNode_AddComponent_PlayerController( f ) \
	f( AddComponent_PlayerController, AddComponent_PlayerController_Inputs, AddComponent_PlayerController_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

#define Player_ScriptNodes( f ) \
	f( ScriptNode_AddComponent_PlayerController ) \
