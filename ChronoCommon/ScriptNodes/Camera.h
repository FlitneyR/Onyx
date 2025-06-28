#pragma once

#include "Common/Scripting/ScriptMacros.h"
#include "Common/ECS/Entity.h"
#include "Common/ECS/CommandBuffer.h"

// AddComponent_Camera
#define AddComponent_Camera_Inputs( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd, = nullptr ) \
	f( onyx::ecs::EntityID, Entity, = onyx::ecs::NoEntity ) \
	f( f32, MinFov, = 1'000.f ) \
	f( f32, MaxFov, = 2'000.f ) \
	f( f32, Margin, = 3.f ) \
	f( f32, MoveSpeed, = 0.5f ) \
	f( f32, ZoomSpeed, = 0.5f ) \

#define AddComponent_Camera_Outputs( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd_Out, = nullptr ) \
	f( onyx::ecs::EntityID, Entity_Out, = onyx::ecs::NoEntity ) \

#define ScriptNode_AddComponent_Camera( f ) \
	f( AddComponent_Camera, AddComponent_Camera_Inputs, AddComponent_Camera_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

// AddComponent_CameraFocus
#define AddComponent_CameraFocus_Inputs( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd, = nullptr ) \
	f( onyx::ecs::EntityID, Entity, = onyx::ecs::NoEntity ) \

#define AddComponent_CameraFocus_Outputs( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd_Out, = nullptr ) \
	f( onyx::ecs::EntityID, Entity_Out, = onyx::ecs::NoEntity ) \

#define ScriptNode_AddComponent_CameraFocus( f ) \
	f( AddComponent_CameraFocus, AddComponent_CameraFocus_Inputs, AddComponent_CameraFocus_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

#define Camera_ScriptNodes( f ) \
	f( ScriptNode_AddComponent_Camera ) \
	f( ScriptNode_AddComponent_CameraFocus ) \
