#pragma once

#include "Common/ECS/Components/SpriteScriptNodes.h"
#include "Common/ECS/Components/TransformScriptNodes.h"
#include "Common/Scripting/CommonScriptNodes.h"

#include "ChronoCommon/ScriptNodes/Camera.h"
#include "ChronoCommon/ScriptNodes/Physics.h"
#include "ChronoCommon/ScriptNodes/Player.h"

#include "Common/Scripting/ScriptMacros.h"

#define SCRIPT_NODES( f ) \
	ONYX_SPRITE_SCRIPT_NODES( f ) \
	ONYX_TRANSFORM2D_SCRIPT_NODES( f ) \
	ONYX_COMMON_SCRIPT_NODES( f ) \
	Camera_ScriptNodes( f ) \
	Physics_ScriptNodes( f ) \
	Player_ScriptNodes( f ) \

namespace onyx::ScriptNodes
{

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
