#include "CommonScriptNodes.h"

#include "Common/ECS/World.h"

namespace onyx
{

//namespace ScriptNodes
//{
//
//ONYX_COMMON_SCRIPT_NODES( SCRIPT_NODE_DECL )
//
//}

SCRIPT_NODE_IMPL( BranchNode )
{
	return inputs.condition ? True : False;
}

SCRIPT_NODE_IMPL( NoEntity )
{
	outputs.entity = onyx::ecs::NoEntity;
	return Then;
}

}
