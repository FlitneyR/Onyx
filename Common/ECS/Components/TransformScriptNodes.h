#pragma once
#include "Common/Scripting/ScriptMacros.h"
#include "Transform.h"

// MakeTransform2D
#define MakeTransform2D_Inputs( f )				\
	f( glm::mat3, Locale, = glm::mat3( 1.f ) )	\
	f( glm::vec2, Position, = { 0.f, 0.f } )	\
	f( glm::vec2, Scale, = { 1.f, 1.f } )		\
	f( f32, Rotation, = 0.f )					\

#define MakeTransform2D_Outputs( f )				\
	f( onyx::Transform2D*, Transform2D, = nullptr )	\

#define MakeTransform2D_Extras \
	onyx::Transform2D Transform = {}; \

#define ScriptNode_MakeTransform2D( f ) \
	f( MakeTransform2D, MakeTransform2D_Inputs, MakeTransform2D_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, MakeTransform2D_Extras )

// AddComponent_Transform2D
#define AddComponent_Transform2D_Inputs( f )				\
	f( onyx::ecs::CommandBuffer*, Cmd, = nullptr )			\
	f( onyx::ecs::EntityID, Entity, = onyx::ecs::NoEntity )	\
	f( onyx::Transform2D*, Transform2D, = nullptr )			\

#define AddComponent_Transform2D_Outputs( f )					\
	f( onyx::ecs::CommandBuffer*, Cmd_Out, = nullptr )			\
	f( onyx::ecs::EntityID, Entity_Out, = onyx::ecs::NoEntity )	\

#define ScriptNode_AddComponent_Transform2D( f ) \
	f( AddComponent_Transform2D, AddComponent_Transform2D_Inputs, AddComponent_Transform2D_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

#define ONYX_TRANSFORM2D_SCRIPT_NODES( f )		\
	f( ScriptNode_MakeTransform2D )				\
	f( ScriptNode_AddComponent_Transform2D )	\

// AddComponent_Attachment
#define AddComponent_Attachment_Inputs( f )							\
	f( onyx::ecs::CommandBuffer*, Cmd, = nullptr )					\
	f( onyx::ecs::EntityID, Entity, = onyx::ecs::NoEntity )			\
	f( onyx::ecs::EntityID, ParentEntity, = onyx::ecs::NoEntity )	\

#define AddComponent_Attachment_Outputs( f )					\
	f( onyx::ecs::CommandBuffer*, Cmd_Out, = nullptr )			\
	f( onyx::ecs::EntityID, Entity_Out, = onyx::ecs::NoEntity )	\

#define ScriptNode_AddComponent_Attachment( f ) \
	f( AddComponent_Attachment, AddComponent_Attachment_Inputs, AddComponent_Attachment_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

#define ONYX_TRANSFORM2D_SCRIPT_NODES( f )		\
	f( ScriptNode_MakeTransform2D )				\
	f( ScriptNode_AddComponent_Transform2D )	\
	f( ScriptNode_AddComponent_Attachment )		\
