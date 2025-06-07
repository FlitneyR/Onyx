#pragma once

#include "Common/Scripting/ScriptMacros.h"
#include "Common/Scripting/CommonScriptNodes.h"

#define SCRIPT_NODES( f )			\
	ONYX_COMMON_SCRIPT_NODES( f )	\

namespace onyx
{

namespace ScriptNodes
{

SCRIPT_NODES( SCRIPT_NODE_DECL );

enum struct Enum : u32
{
	None = -1,
	SCRIPT_NODES( SCRIPT_NODE_ENUM )
	Count
};

Enum EnumFromName( u32 name_hash );
const char* NameFromEnum( Enum e );
std::unique_ptr< IScriptNode > ConstructFromEnum( Enum e );

}

}
