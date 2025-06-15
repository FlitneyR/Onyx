#include "ScriptNodes.h"

#include "Common/Scripting/CommonScriptNodesImpl.h"
#include "Common/Components/TransformScriptNodesImpl.h"

namespace onyx::ScriptNodes
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

std::unique_ptr< IScriptNode > DoCreateScriptNodeCombo()
{
	static std::vector< Enum > sorted_enums;

	if ( sorted_enums.empty() )
	{
		for ( u32 script_node = 0; script_node < (u32)Enum::Count; ++script_node )
			sorted_enums.push_back( (Enum)script_node );

		std::sort( sorted_enums.begin(), sorted_enums.end(), []( const Enum& lhs, const Enum& rhs ) {
			return strcmp( s_names[ (u32)lhs ], s_names[ (u32)rhs ] ) < 0;
		} );
	}

	static char search_term[ 128 ] = "";
	ImGui::InputText( "Search", search_term, _countof( search_term ) );

	if ( ImGui::BeginChild( "##Nodes", { 0, 300 } ) )
	{
		for ( const Enum& script_node : sorted_enums )
		{
			if ( strstr( s_names[ (u32)script_node ], search_term ) && ImGui::Selectable( s_names[ (u32)script_node ] ) )
			{
				ImGui::EndChild();

				search_term[ 0 ] = 0;
				return ConstructFromEnum( (Enum)script_node );
			}
		}

		ImGui::EndChild();
	}

	return nullptr;
}

std::unique_ptr< IScriptNode > ConstructFromTypeHash( u32 type_name_hash )
{
	return ConstructFromEnum( EnumFromName( type_name_hash ) );
}

}
