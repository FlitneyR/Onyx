#pragma once

#include "ScriptMacros.h"

#include "Common/ECS/Components/Transform.h"

// forward declarations
namespace onyx::ecs
{
typedef u32 EntityID;
extern const EntityID NoEntity;
struct CommandBuffer;
}

#define __SCRIPT_NODE_ADD_CONTEXT_GENERIC_INPUTS( f ) \
	f( onyx::ScriptContext*, Ctx, = nullptr ) \
	f( const char*, Name, = nullptr ) \

#define __SCRIPT_NODE_ADD_CONTEXT_GENERIC_OUTPUTS( f ) \
	f( onyx::ScriptContext*, Ctx_Out, = nullptr ) \

// AddContext_AssetLoader
#define AddContext_AssetLoader_Inputs( f ) \
	__SCRIPT_NODE_ADD_CONTEXT_GENERIC_INPUTS( f ) \
	f( onyx::AssetLoader*, AssetLoader, = nullptr ) \

#define ScriptNode_AddContext_AssetLoader( f ) \
	f( AddContext_AssetLoader, AddContext_AssetLoader_Inputs, __SCRIPT_NODE_ADD_CONTEXT_GENERIC_OUTPUTS, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

// AddContext_Cmd
#define AddContext_Cmd_Inputs( f ) \
	__SCRIPT_NODE_ADD_CONTEXT_GENERIC_INPUTS( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd, = nullptr ) \

#define ScriptNode_AddContext_Cmd( f ) \
	f( AddContext_Cmd, AddContext_Cmd_Inputs, __SCRIPT_NODE_ADD_CONTEXT_GENERIC_OUTPUTS, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

// AddContext_String
#define AddContext_String_Inputs( f ) \
	__SCRIPT_NODE_ADD_CONTEXT_GENERIC_INPUTS( f ) \
	f( const char*, String, = nullptr ) \

#define ScriptNode_AddContext_String( f ) \
	f( AddContext_String, AddContext_String_Inputs, __SCRIPT_NODE_ADD_CONTEXT_GENERIC_OUTPUTS, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )


// AddContext_Transform
#define AddContext_Transform2D_Inputs( f ) \
	__SCRIPT_NODE_ADD_CONTEXT_GENERIC_INPUTS( f ) \
	f( onyx::Transform2D*, Transform2D, = nullptr ) \

#define ScriptNode_AddContext_Transform2D( f ) \
	f( AddContext_Transform, AddContext_Transform2D_Inputs, __SCRIPT_NODE_ADD_CONTEXT_GENERIC_OUTPUTS, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

// AddContext_Vec2
#define AddContext_Vec2_Inputs( f ) \
	__SCRIPT_NODE_ADD_CONTEXT_GENERIC_INPUTS( f ) \
	f( glm::vec2, Vec2, = {} ) \

#define ScriptNode_AddContext_Vec2( f ) \
	f( AddContext_Vec2, AddContext_Vec2_Inputs, __SCRIPT_NODE_ADD_CONTEXT_GENERIC_OUTPUTS, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )


// AddEntity
#define ADD_ENTITY_INPUTS( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd, = nullptr ) \

#define ADD_ENTITY_OUTPUTS( f ) \
	f( onyx::ecs::CommandBuffer*, Cmd_Out, = nullptr ) \
	f( onyx::ecs::EntityID, Entity, = onyx::ecs::NoEntity ) \

#define ScriptNode_AddEntity( f ) \
	f( AddEntity, ADD_ENTITY_INPUTS, ADD_ENTITY_OUTPUTS, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

// BoolLiteral
#define BoolLiteral_Extras \
	bool value = false; \

#define BoolLiteral_Outputs( f ) \
	f( bool, Value, = false ) \

#define ScriptNode_BoolLiteral( f ) \
	f( BoolLiteral, SCRIPT_NODE_DEFAULT_INPUTS, BoolLiteral_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, BoolLiteral_Extras )

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

// CopyContext
#define CopyContext_Inputs( f ) \
	f( onyx::ScriptContext*, Ctx, = nullptr ) \

#define CopyContext_Outputs( f ) \
	f( onyx::ScriptContext*, Ctx_Out, = nullptr ) \
	f( onyx::ScriptContext*, Ctx_Copy, = nullptr ) \

#define CopyContext_Extras \
	onyx::ScriptContext ctx;

#define ScriptNode_CopyContext( f ) \
	f( CopyContext, CopyContext_Inputs, CopyContext_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, CopyContext_Extras )

// FloatLiteral
#define FLOAT_LITERAL_SCRIPT_NODE_EXTRAS	\
	f32 Value = 0.f;						\

#define FLOAT_LITERAL_NODE_OUTPUTS( f )	\
	f( f32, Value, = 0.f )				\

#define ScriptNode_FloatLiteral( f ) \
	f( FloatLiteral, SCRIPT_NODE_DEFAULT_INPUTS, FLOAT_LITERAL_NODE_OUTPUTS, SCRIPT_NODE_DEFAULT_EXEC_PINS, FLOAT_LITERAL_SCRIPT_NODE_EXTRAS )

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

// MakeScriptContext
#define MakeScriptContext_Extras \
	onyx::ScriptContext ctx;

#define MakeScriptContext_Outputs( f ) \
	f( onyx::ScriptContext*, Ctx, = nullptr ) \

#define ScriptNode_MakeScriptContext( f ) \
	f( MakeScriptContext, SCRIPT_NODE_DEFAULT_INPUTS, MakeScriptContext_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, MakeScriptContext_Extras )

// MakeVec2
#define MakeVec2_Inputs( f ) \
	f( f32, X, = 0.f ) \
	f( f32, Y, = 0.f ) \

#define MakeVec2_Outputs( f ) \
	f( glm::vec2, Vec2, = {} ) \

#define ScriptNode_MakeVec2( f ) \
	f( MakeVec2, MakeVec2_Inputs, MakeVec2_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

// RunScript
#define RunScript_Inputs( f ) \
	f( onyx::AssetLoader*, AssetLoader, = nullptr) \
	f( const char*, ScriptPath, = nullptr ) \
	f( onyx::ScriptContext*, Ctx, = nullptr ) \

#define RunScript_Outputs( f ) \
	f( onyx::ScriptContext*, Ctx_Out, = nullptr ) \
	f( bool, Succeeded, = false ) \

#define ScriptNode_RunScript( f ) \
	f( RunScript, RunScript_Inputs, RunScript_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

// StringLiteral
#define STRING_LITERAL_SCRIPT_NODE_EXTRAS	\
	std::string String = "";				\

#define SCRIPT_NODE_STRING_LITERAL_OUTPUTS( f )	\
	f( const char*, String, = "" )				\

#define ScriptNode_StringLiteral( f ) \
	f( StringLiteral, SCRIPT_NODE_DEFAULT_INPUTS, SCRIPT_NODE_STRING_LITERAL_OUTPUTS, SCRIPT_NODE_DEFAULT_EXEC_PINS, STRING_LITERAL_SCRIPT_NODE_EXTRAS )

#define ONYX_COMMON_SCRIPT_NODES( f )		\
	f( ScriptNode_AddContext_AssetLoader )	\
	f( ScriptNode_AddContext_Cmd )			\
	f( ScriptNode_AddContext_String )		\
	f( ScriptNode_AddContext_Transform2D )	\
	f( ScriptNode_AddContext_Vec2 )			\
	f( ScriptNode_AddEntity )				\
	f( ScriptNode_BoolLiteral )				\
	f( ScriptNode_Branch )					\
	f( ScriptNode_CopyContext )				\
	f( ScriptNode_FloatLiteral )			\
	f( ScriptNode_IntLiteral )				\
	f( ScriptNode_IsValidEntity )			\
	f( ScriptNode_LogInfoMessage )			\
	f( ScriptNode_LogWarningMessage )		\
	f( ScriptNode_LogErrorMessage )			\
	f( ScriptNode_MakeScriptContext )		\
	f( ScriptNode_MakeVec2 )				\
	f( ScriptNode_RunScript )				\
	f( ScriptNode_StringLiteral )			\
