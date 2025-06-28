#pragma once

#include "ChronoCommon/Components/Physics.h"
#include "Common/Scripting/ScriptMacros.h"

// AddComponent_PhysicsBody
#define AddComponent_PhysicsBody_Inputs( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd, = nullptr ) \
	f( onyx::ecs::EntityID, Entity, = onyx::ecs::NoEntity ) \
	f( f32, LinearFriction, = 0.f ) \
	f( f32, AngularFriction, = 0.f ) \

#define AddComponent_PhysicsBody_Outputs( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd_Out, = nullptr ) \
	f( onyx::ecs::EntityID, Entity_Out, = onyx::ecs::NoEntity ) \

#define ScriptNode_AddComponent_PhysicsBody( f ) \
	f( AddComponent_PhysicsBody, AddComponent_PhysicsBody_Inputs, AddComponent_PhysicsBody_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

// AddComponent_Collider
#define AddComponent_Collider_Inputs( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd, = nullptr ) \
	f( onyx::ecs::EntityID, Entity, = onyx::ecs::NoEntity ) \
	f( f32, Radius, = 100.f ) \

#define AddComponent_Collider_Outputs( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd_Out, = nullptr ) \
	f( onyx::ecs::EntityID, Entity_Out, = onyx::ecs::NoEntity ) \

#define ScriptNode_AddComponent_Collider( f ) \
	f( AddComponent_Collider, AddComponent_Collider_Inputs, AddComponent_Collider_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

#define Physics_ScriptNodes( f ) \
	f( ScriptNode_AddComponent_PhysicsBody ) \
	f( ScriptNode_AddComponent_Collider ) \
