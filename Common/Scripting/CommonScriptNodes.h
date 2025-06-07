#pragma once

#include "ScriptMacros.h"

#define BRANCH_NODE_INPUTS( f )	\
	f( bool, condition )		\

#define BRANCH_NODE_EXEC_PINS( f )	\
	f( True )						\
	f( False )						\

#define BranchNode( f ) f( BranchNode, BRANCH_NODE_INPUTS, SCRIPT_NODE_DEFAULT_OUTPUTS, BRANCH_NODE_EXEC_PINS )

#define NO_ENTITY_OUTPUTS( f ) \
	f( onyx::ecs::EntityID, entity )

#define NoEntity( f ) f( NoEntity, SCRIPT_NODE_DEFAULT_INPUTS, NO_ENTITY_OUTPUTS, SCRIPT_NODE_DEFAULT_EXEC_PINS )

#define ONYX_COMMON_SCRIPT_NODES( f )	\
	f( BranchNode )						\
	f( NoEntity )						\
