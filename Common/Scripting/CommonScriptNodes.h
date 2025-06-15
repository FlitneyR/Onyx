#pragma once

#include "ScriptMacros.h"

// forward declarations
namespace onyx::ecs
{
typedef u32 EntityID;
extern const EntityID NoEntity;
struct CommandBuffer;
}

// AddEntity
#define ADD_ENTITY_INPUTS( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd, = nullptr ) \

#define ADD_ENTITY_OUTPUTS( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd_Out, = nullptr ) \
	f( onyx::ecs::EntityID, Entity, = onyx::ecs::NoEntity ) \

#define ScriptNode_AddEntity( f ) \
	f( AddEntity, ADD_ENTITY_INPUTS, ADD_ENTITY_OUTPUTS, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

// Branch
#define BRANCH_NODE_INPUTS( f )	\
	f( bool, Condition, = false )	\

#define BRANCH_NODE_OUTPUTS( f )	\
	f( bool, PassThrough, = false )	\

#define BRANCH_NODE_EXEC_PINS( f )	\
	f( True )						\
	f( False )						\

#define ScriptNode_Branch( f ) \
	f( Branch, BRANCH_NODE_INPUTS, BRANCH_NODE_OUTPUTS, BRANCH_NODE_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

// IntLiteral
#define INT_LITERAL_SCRIPT_NODE_EXTRAS	\
	i32 Value = 0;						\

#define INT_LITERAL_NODE_OUTPUTS( f )	\
	f( i32, Value, = 0 )				\

#define ScriptNode_IntLiteral( f ) \
	f( IntLiteral, SCRIPT_NODE_DEFAULT_INPUTS, INT_LITERAL_NODE_OUTPUTS, SCRIPT_NODE_DEFAULT_EXEC_PINS, INT_LITERAL_SCRIPT_NODE_EXTRAS )

// IsValidEntity
#define IS_VALID_ENTITY_INPUTS( f ) \
	f( onyx::ecs::EntityID, Entity, = onyx::ecs::NoEntity )

#define IS_VALID_ENTITY_OUTPUTS( f ) \
	f( bool, IsValid, = false )

#define IS_VALID_ENTITY_EXEC_PINS( f ) \
	f( Valid ) \
	f( NotValid ) \

#define ScriptNode_IsValidEntity( f ) \
	f( IsValidEntity, IS_VALID_ENTITY_INPUTS, IS_VALID_ENTITY_OUTPUTS, IS_VALID_ENTITY_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

// Log_Message
#define SCRIPT_NODE_LOG_MESSAGE_INPUTS( f )	\
	f( const char*, Message, = "" )			\

#define ScriptNode_LogInfoMessage( f ) \
	f( LogInfoMessage, SCRIPT_NODE_LOG_MESSAGE_INPUTS, SCRIPT_NODE_DEFAULT_OUTPUTS, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

#define ScriptNode_LogWarningMessage( f ) \
	f( LogWarningMessage, SCRIPT_NODE_LOG_MESSAGE_INPUTS, SCRIPT_NODE_DEFAULT_OUTPUTS, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

#define ScriptNode_LogErrorMessage( f ) \
	f( LogErrorMessage, SCRIPT_NODE_LOG_MESSAGE_INPUTS, SCRIPT_NODE_DEFAULT_OUTPUTS, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

// StringLiteral
#define STRING_LITERAL_SCRIPT_NODE_EXTRAS	\
	std::string String = "";				\

#define SCRIPT_NODE_STRING_LITERAL_OUTPUTS( f )	\
	f( const char*, String, = "" )				\

#define ScriptNode_StringLiteral( f ) \
	f( StringLiteral, SCRIPT_NODE_DEFAULT_INPUTS, SCRIPT_NODE_STRING_LITERAL_OUTPUTS, SCRIPT_NODE_DEFAULT_EXEC_PINS, STRING_LITERAL_SCRIPT_NODE_EXTRAS )

#define ONYX_COMMON_SCRIPT_NODES( f )		\
	f( ScriptNode_AddEntity )				\
	f( ScriptNode_Branch )					\
	f( ScriptNode_IntLiteral )				\
	f( ScriptNode_IsValidEntity )			\
	f( ScriptNode_LogInfoMessage )			\
	f( ScriptNode_LogWarningMessage )		\
	f( ScriptNode_LogErrorMessage )			\
	f( ScriptNode_StringLiteral )			\
