#include "ScriptNodes.h"
#include "Common/Scripting/CommonScriptNodesImpl.h"

namespace onyx
{

namespace ScriptNodes
{

Enum EnumFromName( u32 name_hash )
{
	switch ( name_hash )
	{
		SCRIPT_NODES( SCRIPT_NODE_ENUM_FROM_NAME_CASE )
	default: return Enum::None;
	}
}

static const char* const s_names[] = { SCRIPT_NODES( SCRIPT_NODE_NAME ) };
const char* NameFromEnum( Enum e ) { return e >= Enum::Count ? nullptr : s_names[ (u32)e ]; }

std::unique_ptr< IScriptNode > ConstructFromEnum( Enum e )
{
	switch ( e )
	{
		SCRIPT_NODES( SCRIPT_NODE_CONSTRUCT_FROM_ENUM_CASE )
	default: return nullptr;
	}
}

}

}
