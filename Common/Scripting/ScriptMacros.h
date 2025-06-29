#pragma once

#include "Script.h"

namespace onyx
{

#define SCRIPT_NODE_DEFAULT_INPUTS( f ) f( nullptr_t, __ignore, = nullptr )
#define SCRIPT_NODE_DEFAULT_OUTPUTS( f ) f( nullptr_t, __ignore, = nullptr )
#define SCRIPT_NODE_DEFAULT_EXEC_PINS( f ) f( Then )
#define SCRIPT_NODE_NO_EXTRA_MEMBERS

#define SCRIPT_NODE_PIN_SET_GET_DATA_CASE( type, name, ... ) case #name##_name: return &name;
#define SCRIPT_NODE_PIN_SET_COPY_DATA_CASE( type, name, ... ) case #name##_name: name = *static_cast< type* >( source ); return;
#define SCRIPT_NODE_PIN_SET_NAME( type, name, ... ) #name,
#define SCRIPT_NODE_PIN_SET_HASH( type, name, ... ) #name##_name,
#define SCRIPT_NODE_PIN_SET_GET_TYPE_NAME_CASE( type, name, ... ) case #name##_name: return #type;
#define SCRIPT_NODE_PIN_SET_GET_TYPE_ID_CASE( type, name, ... ) \
	case #name##_name: { const static size_t result = typeid( type ).hash_code(); return &result; }

#define SCRIPT_NODE_PIN_SET_IMPL( struct_name, pin_set )							\
	void* struct_name::GetPinData( BjSON::NameHash pin_name )						\
	{																				\
		switch( pin_name )															\
		{																			\
			pin_set( SCRIPT_NODE_PIN_SET_GET_DATA_CASE )							\
			default: return nullptr;												\
		}																			\
	}																				\
	void struct_name::CopyPinData( BjSON::NameHash pin_name, void* source )			\
	{																				\
		switch( pin_name )															\
		{																			\
			pin_set( SCRIPT_NODE_PIN_SET_COPY_DATA_CASE )							\
			default: break;															\
		}																			\
	}																				\
	const char* const* struct_name::GetPinNames( u32& count ) const					\
	{																				\
		static const char* const s_names[] {										\
			pin_set( SCRIPT_NODE_PIN_SET_NAME )										\
		};																			\
		if ( !strcmp( s_names[ 0 ], "__ignore" ) )									\
		{																			\
			count = 0;																\
			return nullptr;															\
		}																			\
		count = _countof( s_names );												\
		return s_names;																\
	}																				\
	const BjSON::NameHash* struct_name::GetPinNameHashes( u32& count ) const		\
	{																				\
		static BjSON::NameHash s_names[] {											\
			pin_set( SCRIPT_NODE_PIN_SET_HASH )										\
		};																			\
		if ( s_names[ 0 ] == "__ignore"_name )										\
		{																			\
			count = 0;																\
			return nullptr;															\
		}																			\
		count = _countof( s_names );												\
		return s_names;																\
	}																				\
	const size_t* struct_name::GetPinTypeID( BjSON::NameHash pin_name ) const		\
	{																				\
		switch( pin_name )															\
		{																			\
			pin_set( SCRIPT_NODE_PIN_SET_GET_TYPE_ID_CASE )							\
			default: return nullptr;												\
		}																			\
	}																				\
	const char* struct_name::GetPinTypeName( BjSON::NameHash pin_name ) const		\
	{																				\
		switch( pin_name )															\
		{																			\
			pin_set( SCRIPT_NODE_PIN_SET_GET_TYPE_NAME_CASE )						\
			default: return nullptr;												\
		}																			\
	}																				\
	u32 struct_name::GetPinIndex( BjSON::NameHash pin_name ) const					\
	{																				\
		u32 count;																	\
		const BjSON::NameHash* hashes = GetPinNameHashes( count );					\
																					\
		for ( u32 index = 0; index < count; ++index )								\
			if ( hashes[ index ] == pin_name )										\
				return index;														\
																					\
		return ~0u;																	\
	}																				\

#define SCRIPT_NODE_PIN_SETS_IMPL( node, inputs, outputs, _exec_pins, _extras )	\
	SCRIPT_NODE_PIN_SET_IMPL( ScriptNodes::node::Inputs, inputs )				\
	SCRIPT_NODE_PIN_SET_IMPL( ScriptNodes::node::Outputs, outputs )				\

#define SCRIPT_NODE_PIN_STRUCT_MEMBER( type, name, ... ) type name __VA_ARGS__;

#define SCRIPT_NODE_EXTRA_FUNCTIONS( node, _inputs, _outputs, _exec_pins, _extras ) \
	void ScriptNodes::node::DoCustomUI()											\
	{ onyx::ScriptNodes_DoCustomUI( *this ); }										\
	void ScriptNodes::node::DoCustomLoad( const BjSON::IReadOnlyObject& reader )	\
	{ onyx::ScriptNodes_DoCustomLoad( *this, reader ); }							\
	void ScriptNodes::node::DoCustomSave( BjSON::IReadWriteObject& writer ) 		\
	{ onyx::ScriptNodes_DoCustomSave( *this, writer ); }							\

#define SCRIPT_NODE_FUNCTION_DECL( node, _inputs, _outputs, exec_pins, _extras ) \
	ScriptNodes::node::ExecPin ScriptNodes::node::Exec( ScriptContext& ctx, const Inputs& inputs, Outputs& outputs, Extras& extras )

#define EXEC_PIN_ENUM( pin ) pin,
#define EXEC_PIN_NAME( pin ) #pin,
#define EXEC_PIN_HASH( pin ) #pin##_name,

#define SCRIPT_NODE_WRAPPER_DECL( node, _inputs, _outputs, exec_pins, extras )				\
	struct node : IScriptNode																\
	{																						\
		struct Extras																		\
		{																					\
			extras																			\
		} m_extras;																			\
																							\
		struct Inputs : IScriptNodePinSet													\
		{																					\
			_inputs( SCRIPT_NODE_PIN_STRUCT_MEMBER )										\
			const char* const* GetPinNames( u32& count ) const override;					\
			const BjSON::NameHash* GetPinNameHashes( u32& count ) const override;			\
			void* GetPinData( BjSON::NameHash pin_name ) override;							\
			void CopyPinData( BjSON::NameHash pin_name, void* source ) override;			\
			const size_t* GetPinTypeID( BjSON::NameHash pin_name ) const override;			\
			const char* GetPinTypeName( BjSON::NameHash pin_name ) const override;			\
			u32 GetPinIndex( BjSON::NameHash pin_name ) const override;						\
		} m_inputs;																			\
																							\
		struct Outputs : IScriptNodePinSet													\
		{																					\
			_outputs( SCRIPT_NODE_PIN_STRUCT_MEMBER )										\
			const char* const* GetPinNames( u32& count ) const override;					\
			const BjSON::NameHash* GetPinNameHashes( u32& count ) const override;			\
			void* GetPinData( BjSON::NameHash pin_name ) override;							\
			void CopyPinData( BjSON::NameHash pin_name, void* source ) override;			\
			const size_t* GetPinTypeID( BjSON::NameHash pin_name ) const override;			\
			const char* GetPinTypeName( BjSON::NameHash pin_name ) const override;			\
			u32 GetPinIndex( BjSON::NameHash pin_name ) const override;						\
		} m_outputs;																		\
																							\
		const char* GetScriptNodeTypeName() const override { return #node; }				\
																							\
		IScriptNodePinSet& GetInputs() override { return m_inputs; }						\
																							\
		IScriptNodePinSet& GetOutputs() override { return m_outputs; }						\
																							\
		u32 Exec( ScriptContext& ctx ) override												\
		{																					\
			return Exec( ctx, m_inputs, m_outputs, m_extras );								\
		}																					\
																							\
		enum ExecPin : u32 { Failed = ~0u, exec_pins( EXEC_PIN_ENUM ) Count };				\
																							\
		inline static const char* const s_ExecPin_Names[] = { exec_pins( EXEC_PIN_NAME ) };	\
																							\
		const char* const* GetExecPinNames( u32& count ) const override						\
		{ count = ExecPin::Count; return s_ExecPin_Names; }									\
																							\
		void DoCustomUI() override;															\
		void DoCustomLoad( const BjSON::IReadOnlyObject& reader ) override;					\
		void DoCustomSave( BjSON::IReadWriteObject& writer ) override;						\
																							\
		u32 GetExecPinIndex( u32 name_hash ) const override									\
		{																					\
			static const BjSON::NameHash s_execPinHashes[] { exec_pins( EXEC_PIN_HASH ) };	\
			for ( u32 index = 0; index < ExecPin::Count; ++index )							\
				if ( s_execPinHashes[ index ] == name_hash )								\
					return index;															\
			return ~0u;																		\
		}																					\
	private:																				\
		static ExecPin Exec(																\
			ScriptContext& ctx,																\
			const Inputs& inputs,															\
			Outputs& outputs,																\
			Extras& extra																	\
		);																					\
	}																						\

#define SCRIPT_NODE_DECL( script ) \
	script( SCRIPT_NODE_WRAPPER_DECL ); \

#define SCRIPT_NODE_IMPL( script )			\
	script( SCRIPT_NODE_PIN_SETS_IMPL )		\
	script( SCRIPT_NODE_EXTRA_FUNCTIONS)	\
	script( SCRIPT_NODE_FUNCTION_DECL )		\

#define _SCRIPT_NODE_CUSTOM_UI( node, _inputs, _outputs, _exec_pins, _extras ) \
	void ScriptNodes_DoCustomUI( ScriptNodes::node& self )

#define _SCRIPT_NODE_CUSTOM_LOAD( node, _inputs, _outputs, _exec_pins, _extras ) \
	void ScriptNodes_DoCustomLoad( ScriptNodes::node& self, const BjSON::IReadOnlyObject& reader )

#define _SCRIPT_NODE_CUSTOM_SAVE( node, _inputs, _outputs, _exec_pins, _extras ) \
	void ScriptNodes_DoCustomSave( ScriptNodes::node& self, BjSON::IReadWriteObject& writer )

#define SCRIPT_NODE_CUSTOM_UI( script ) script( _SCRIPT_NODE_CUSTOM_UI )
#define SCRIPT_NODE_CUSTOM_LOAD( script ) script( _SCRIPT_NODE_CUSTOM_LOAD )
#define SCRIPT_NODE_CUSTOM_SAVE( script ) script( _SCRIPT_NODE_CUSTOM_SAVE )

#define SCRIPT_NODE_ENUM_INTERNAL( script, _inputs, _outputs, _exec_pins, _extras ) script
#define SCRIPT_NODE_ENUM( script ) script( SCRIPT_NODE_ENUM_INTERNAL ),

#define SCRIPT_NODE_NAME_INTERNAL( script, _inputs, _outputs, _exec_pins, _extras ) #script
#define SCRIPT_NODE_NAME( script ) script( SCRIPT_NODE_NAME_INTERNAL ),

#define SCRIPT_NODE_ENUM_FROM_NAME_INTERNAL( script, _inputs, _outputs, _exec_pins, _extras ) \
	case #script##_name: return Enum::script;
#define SCRIPT_NODE_ENUM_FROM_NAME_CASE( script ) script( SCRIPT_NODE_ENUM_FROM_NAME_INTERNAL )

#define SCRIPT_NODE_CONSTRUCT_FROM_ENUM_CASE_INTERNAL( script, _inputs, _outputs, _exec_pins, _extras ) \
	case Enum::script: return std::make_unique< script >();
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
