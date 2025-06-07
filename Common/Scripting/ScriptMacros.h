#pragma once

#include "Script.h"

namespace onyx
{

#define SCRIPT_NODE_DEFAULT_INPUTS( f ) f( u8, __ignore )
#define SCRIPT_NODE_DEFAULT_OUTPUTS( f ) f( u8, __ignore )
#define SCRIPT_NODE_DEFAULT_EXEC_PINS( f ) f( Then )

#define SCRIPT_NODE_PIN_SET_GET_DATA_CASE( type, name ) case #name##_name: return &name;
#define SCRIPT_NODE_PIN_SET_GET_SIZE_CASE( type, name ) case #name##_name: return sizeof( type );
#define SCRIPT_NODE_PIN_SET_NAME( type, name ) #name,
#define SCRIPT_NODE_PIN_SET_GET_TYPE_CASE( type, name ) case #name##_name: return #type;

#define SCRIPT_NODE_PIN_SET_IMPL( struct_name, pin_set )						\
	void* struct_name::GetPinData( BjSON::NameHash pin_name )					\
	{																			\
		switch( pin_name )														\
		{																		\
			pin_set( SCRIPT_NODE_PIN_SET_GET_DATA_CASE )						\
			default: return nullptr;											\
		}																		\
	}																			\
	u32 struct_name::GetPinSize( BjSON::NameHash pin_name ) const				\
	{																			\
		switch( pin_name )														\
		{																		\
			pin_set( SCRIPT_NODE_PIN_SET_GET_SIZE_CASE )						\
			default: return 0;													\
		}																		\
	}																			\
	const char* const* struct_name::GetPinNames( u32& count ) const				\
	{																			\
		static const char* const s_names[] {									\
			pin_set( SCRIPT_NODE_PIN_SET_NAME )									\
		};																		\
		count = _countof( s_names );											\
		return s_names;															\
	}																			\
	const char* struct_name::GetPinTypeName( BjSON::NameHash pin_name ) const	\
	{																			\
		switch( pin_name )														\
		{																		\
			pin_set( SCRIPT_NODE_PIN_SET_GET_TYPE_CASE )						\
			default: return nullptr;											\
		}																		\
	}																			\

#define SCRIPT_NODE_PIN_SETS_IMPL( node, inputs, outputs, exec_pins )	\
	SCRIPT_NODE_PIN_SET_IMPL( ScriptNodes::node::Inputs, inputs )		\
	SCRIPT_NODE_PIN_SET_IMPL( ScriptNodes::node::Outputs, outputs )		\

#define SCRIPT_NODE_PIN_STRUCT_MEMBER( type, name ) type name;

#define SCRIPT_NODE_FUNCTION_DECL( node, _inputs, _outputs, exec_pins ) \
	ScriptNodes::node::ExecPin ScriptNodes::node::Exec( IScriptContext& ctx, const Inputs& inputs, Outputs& outputs )

#define EXEC_PIN_ENUM( pin ) pin,
#define EXEC_PIN_NAME( pin ) #pin,

#define SCRIPT_NODE_WRAPPER_DECL( node, _inputs, _outputs, exec_pins )								\
	struct node : IScriptNode																		\
	{																								\
		struct Inputs : IScriptNodePinSet															\
		{																							\
			_inputs( SCRIPT_NODE_PIN_STRUCT_MEMBER )												\
			const char* const* GetPinNames( u32& count ) const override;							\
			void* GetPinData( BjSON::NameHash pin_name ) override;									\
			u32 GetPinSize( BjSON::NameHash pin_name ) const override;								\
			const char* GetPinTypeName( BjSON::NameHash pin_name ) const override;					\
		} m_inputs;																					\
		struct Outputs : IScriptNodePinSet															\
		{																							\
			_outputs( SCRIPT_NODE_PIN_STRUCT_MEMBER )												\
			const char* const* GetPinNames( u32& count ) const override;							\
			void* GetPinData( BjSON::NameHash pin_name ) override;									\
			u32 GetPinSize( BjSON::NameHash pin_name ) const override;								\
			const char* GetPinTypeName( BjSON::NameHash pin_name ) const override;					\
		} m_outputs;																				\
		IScriptNodePinSet& GetInputs() override { return m_inputs; }								\
		IScriptNodePinSet& GetOutputs() override { return m_outputs; }								\
		u32 Exec( IScriptContext& ctx ) override { return Exec( ctx, m_inputs, m_outputs ); }	\
		enum ExecPin : u32 { Failed = -1, exec_pins( EXEC_PIN_ENUM ) Count };						\
		inline static const char* const s_ExecPin_Names[] = { exec_pins( EXEC_PIN_NAME ) };			\
	private:																						\
		static ExecPin Exec( IScriptContext& ctx, const Inputs& inputs, Outputs& outputs );		\
	}																								\

#define SCRIPT_NODE_DECL( script ) \
	script( SCRIPT_NODE_WRAPPER_DECL ); \

#define SCRIPT_NODE_IMPL( script ) \
	script( SCRIPT_NODE_PIN_SETS_IMPL ) \
	script( SCRIPT_NODE_FUNCTION_DECL ) \

#define SCRIPT_NODE_ENUM_INTERNAL( script, _inputs, _outputs, _exec_pins ) script
#define SCRIPT_NODE_ENUM( script ) script( SCRIPT_NODE_ENUM_INTERNAL ),

#define SCRIPT_NODE_NAME_INTERNAL( script, _inputs, _outputs, _exec_pins ) #script
#define SCRIPT_NODE_NAME( script ) script( SCRIPT_NODE_NAME_INTERNAL ),

#define SCRIPT_NODE_ENUM_FROM_NAME_INTERNAL( script, _inputs, _outputs, _exec_pins ) case #script##_name: return Enum::script;
#define SCRIPT_NODE_ENUM_FROM_NAME_CASE( script ) script( SCRIPT_NODE_ENUM_FROM_NAME_INTERNAL )

#define SCRIPT_NODE_CONSTRUCT_FROM_ENUM_CASE_INTERNAL( script, _inputs, _outputs, _exec_pins ) case Enum::script: return std::make_unique< script >();
#define SCRIPT_NODE_CONSTRUCT_FROM_ENUM_CASE( script ) script( SCRIPT_NODE_CONSTRUCT_FROM_ENUM_CASE_INTERNAL )

// Example usage: A script node to make a bullet
// 
//	// Declare inputs and outputs in a header file e.g. BulletScriptNodes.h
// 
//	#define MAKE_BULLET_INPUTS( f ) \
//		f( glm::vec3, position )	\
//		f( glm::vec3, velocity )	\
//	
//	#define MAKE_BULLET_OUTPUTS( f )	\
//		f( u32, bullet_entity )			\
//	
//	#define MAKE_BULLET_EXEC_PINS( f )	\
//		f( Then )						\
//
//	#define MakeBullet( f ) f( MakeBullet, MAKE_BULLET_INPUTS, MAKE_BULLET_OUTPUTS, MAKE_BULLET_EXEC_PINS )
// 
//	// Add MakeBullet to ScriptNodes.h
// 
//	...
//	#include "BulletScriptNodes.h"
//	...
// 
//	#define SCRIPT_NODES( f ) \
//		...
//		f( MakeBullet )
//		...
// 
// 
//	// Implement MakeBullet in a source file
//	
//	...
//	#include "BulletScriptNodes.h"
//	...
//	
//	SCRIPT_NODE_IMPL( MakeBullet )
//	{
//		// validate inputs
//	
//		outputs.bullet_entity = ctx.GetECSWorld().AddEntity(
//			Transform::FromPosition( inputs.position ),
//			Velocity { inputs.velocity },
//			// ... add a mesh, rigidbody, etc.
//		);
//	
//		// tell the script runner to go to the next node
//		return Then;
//	}

}
