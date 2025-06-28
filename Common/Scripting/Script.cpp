#include "Script.h"

#include <unordered_set>
#include <algorithm>

#include "Common/LowLevel/LowLevelInterface.h"

#include "imgui_stdlib.h"

namespace onyx
{

ScriptContext::IPin* ScriptContext::GetInput( BjSON::NameHash name )
{
	auto iter = m_inputs.find( name );
	if ( iter == m_inputs.end() )
		return nullptr;

	return iter->second.get();
}

ScriptContext::IPin* ScriptContext::GetOutput( BjSON::NameHash name )
{
	auto iter = m_outputs.find( name );
	if ( iter == m_outputs.end() )
		return nullptr;

	return iter->second.get();
}

void ScriptNodes_DoCustomUI( IScriptNode& node ) {}
void ScriptNodes_DoCustomLoad( IScriptNode& node, const BjSON::IReadOnlyObject& reader ) {}
void ScriptNodes_DoCustomSave( IScriptNode& node, BjSON::IReadWriteObject& writer ) {}

void Script::Load( LoadType type )
{
	m_loadingState = LoadingState::Loading;

	const BjSON::IReadOnlyObject* const reader = GetReader();

	if ( !WEAK_ASSERT( reader, "We can't start reading until we have a reader!" ) )
	{
		m_loadingState = LoadingState::Errored;
		return;
	}

	u32 asset_type = 0;
	WEAK_ASSERT( reader->GetLiteral( "__assetType"_name, asset_type ) == sizeof( asset_type ) );

	if ( !WEAK_ASSERT( asset_type == "Script"_name, "Trying to load a script, but the file isn't a script" ) )
	{
		m_loadingState = LoadingState::Errored;
		return;
	}

	if ( !WEAK_ASSERT( reader->GetLiteral( "entry_point"_name, m_entryPoint ) == sizeof( m_entryPoint ) ) )
	{
		m_loadingState = LoadingState::Errored;
		return;
	}

	if ( std::shared_ptr< const BjSON::IReadOnlyObjectArray > inputs_reader = reader->GetArray( "inputs"_name ) )
	{
		for ( u32 index = 0; index < inputs_reader->Count(); ++index )
			if ( std::shared_ptr< const BjSON::IReadOnlyObject > input = inputs_reader->GetChild( index ) )
				m_inputs.insert( { input->GetLiteral< std::string >( "name"_name ), 0 } );
	}

	std::shared_ptr< const BjSON::IReadOnlyObjectArray > nodes_reader = reader->GetArray( "nodes"_name );
	if ( !WEAK_ASSERT( nodes_reader ) )
	{
		m_loadingState = LoadingState::Errored;
		return;
	}

	m_nodes.reserve( nodes_reader->Count() );
	for ( u32 node_index = 0; node_index < nodes_reader->Count(); ++node_index )
	{
		std::shared_ptr< const BjSON::IReadOnlyObject > node_reader = nodes_reader->GetChild( node_index );

		std::string name;
		if ( !WEAK_ASSERT( node_reader->GetLiteral( "name"_name, name ) ) )
		{
			m_loadingState = LoadingState::Errored;
			return;
		}

		NodeWrapper node( name.c_str() );

		if ( !m_nodes.empty() && !WEAK_ASSERT( node.m_nameHash > m_nodes.back()->m_nameHash, "Nodes were not serialised in order" ) )
		{
			m_loadingState = LoadingState::Errored;
			return;
		}

		WEAK_ASSERT( node_reader->GetLiteral( "position"_name, node.m_position ) == sizeof( node.m_position ) );

		u32 type = 0;
		if ( !WEAK_ASSERT( node_reader->GetLiteral( "type"_name, type ) == sizeof( type ) ) )
		{
			m_loadingState = LoadingState::Errored;
			return;
		}

		node.m_node = ScriptNodes::ConstructFromTypeHash( type );
		if ( !WEAK_ASSERT( node.m_node != nullptr ) )
		{
			m_loadingState = LoadingState::Errored;
			return;
		}

		u32 input_pin_count;
		u32 exec_pin_count;
		const char* const* input_pin_names = node.m_node->GetInputs().GetPinNames( input_pin_count );
		const char* const* exec_pin_names = node.m_node->GetExecPinNames( exec_pin_count );

		node.m_inputLinks.resize( input_pin_count );
		node.m_execLinks.resize( exec_pin_count );

		std::shared_ptr< const BjSON::IReadOnlyObject > input_pins_reader = node_reader->GetChild( "input_pins"_name );
		std::shared_ptr< const BjSON::IReadOnlyObject > exec_pins_reader = node_reader->GetChild( "exec_pins"_name );

		for ( u32 index = 0; index < input_pin_count; ++index )
		{
			if ( auto input_pin_reader = input_pins_reader->GetChild( BjSON::HashName( input_pin_names[ index ] ) ) )
			{
				input_pin_reader->GetLiteral( "node"_name, node.m_inputLinks[ index ].m_sourceNodeNameHash );
				input_pin_reader->GetLiteral( "pin"_name, node.m_inputLinks[ index ].m_sourcePin );
			}
		}

		for ( u32 index = 0; index < exec_pin_count; ++index )
		{
			if ( auto exec_pin_reader = exec_pins_reader->GetChild( BjSON::HashName( exec_pin_names[ index ] ) ) )
			{
				exec_pin_reader->GetLiteral( "node"_name, node.m_execLinks[ index ].m_nextNodeNameHash );
			}
		}

		if ( auto custom_reader = node_reader->GetChild( "custom"_name ) )
			node.m_node->DoCustomLoad( *custom_reader );

		m_nodes.push_back( std::make_unique< NodeWrapper >( std::move( node ) ) );
	}

	for ( auto& node : m_nodes )
	{
		for ( auto& input_link : node->m_inputLinks )
			if ( auto iter = GetNode( input_link.m_sourceNodeNameHash ); iter != m_nodes.end() && ( *iter )->m_nameHash == input_link.m_sourceNodeNameHash )
				input_link.m_sourceNode = *iter;

		for ( auto& exec_link : node->m_execLinks )
			if ( auto iter = GetNode( exec_link.m_nextNodeNameHash ); iter != m_nodes.end() && ( *iter )->m_nameHash == exec_link.m_nextNodeNameHash )
				exec_link.m_nextNode = *iter;
	}

	m_loadingState = LoadingState::Loaded;
}

void Script::Save( BjSON::IReadWriteObject& writer, SaveType type )
{
	writer.SetLiteral( "__assetType"_name, "Script"_name );
	writer.SetLiteral( "entry_point"_name, m_entryPoint );

	BjSON::IReadWriteObjectArray& inputs_writer = writer.AddArray( "inputs"_name );
	for ( auto& [name, _] : m_inputs )
		inputs_writer.AddChild()
			.SetLiteral( "name"_name, name );

	BjSON::IReadWriteObjectArray& nodes_writer = writer.AddArray( "nodes"_name );
	for ( const std::shared_ptr< NodeWrapper >& node : m_nodes )
	{
		STRONG_ASSERT( node );

		BjSON::IReadWriteObject& node_writer = nodes_writer.AddChild();

		node_writer.SetLiteral( "type"_name, BjSON::HashName( node->m_node->GetScriptNodeTypeName() ) );
		node_writer.SetLiteral( "name"_name, node->m_name );
		node_writer.SetLiteral( "position"_name, node->m_position );

		BjSON::IReadWriteObject& input_pins_writer = node_writer.AddChild( "input_pins"_name );
		BjSON::IReadWriteObject& exec_pins_writer = node_writer.AddChild( "exec_pins"_name );

		u32 input_pin_count;
		u32 exec_pin_count;
		const char* const* input_pin_names = node->m_node->GetInputs().GetPinNames( input_pin_count );
		const char* const* exec_pin_names = node->m_node->GetExecPinNames( exec_pin_count );

		for ( u32 index = 0; index < input_pin_count; ++index )
		{
			const DataLink& input_link = node->m_inputLinks[ index ];

			if ( input_link.m_sourceNodeNameHash == "__entryPoint"_name || input_link.m_sourceNode.lock() )
			{
				input_pins_writer.AddChild( BjSON::HashName( input_pin_names[ index ] ) )
					.SetLiteral( "node"_name, input_link.m_sourceNodeNameHash )
					.SetLiteral( "pin"_name, input_link.m_sourcePin );
			}
		}

		for ( u32 index = 0; index < exec_pin_count; ++index )
		{
			const ExecLink& exec_link = node->m_execLinks[ index ];

			if ( const auto next_node = exec_link.m_nextNode.lock() )
				exec_pins_writer.AddChild( BjSON::HashName( exec_pin_names[ index ] ) )
					.SetLiteral( "node"_name, next_node->m_nameHash );
		}

		auto& custom_writer = node_writer.AddChild( "custom"_name );
		node->m_node->DoCustomSave( custom_writer );
	}
}

void Script::DoAssetManagerButton( const char* name, const char* path, f32 width, std::shared_ptr< IAsset > asset, IFrameContext& frame_context )
{
	switch ( m_loadingState )
	{
	case LoadingState::Loaded:
		ImGui::PushStyleColor( ImGuiCol_Button,			ImVec4( 0.f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered,	ImVec4( 0.f, 0.5f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive,	ImVec4( 0.f, 0.125f, 0.125f, 1.f ) );
		break;
	case LoadingState::Errored:
		ImGui::PushStyleColor( ImGuiCol_Button,			ImVec4( 0.25f, 0.f, 0.f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered,	ImVec4( 0.5f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive,	ImVec4( 0.125f, 0.f, 0.f, 1.f ) );
		break;
	default:
		ImGui::PushStyleColor( ImGuiCol_Button,			ImVec4( 0.25f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered,	ImVec4( 0.25f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive,	ImVec4( 0.125f, 0.125f, 0.125f, 1.f ) );
		break;
	}
	
	if ( ImGui::Button( name ) )
	{
		if ( m_loadingState == LoadingState::Unloaded )
			Load( IAsset::LoadType::Editor );

		ScriptEditor* const window = editor::AddWindow< ScriptEditor >();
		window->m_script = std::static_pointer_cast< Script >( asset );
		window->m_scriptPath = path;
	}

	ImGui::PopStyleColor( 3 );
}

std::vector< std::shared_ptr< Script::NodeWrapper > >::iterator Script::GetNode( BjSON::NameHash name_hash )
{
	return std::lower_bound( m_nodes.begin(), m_nodes.end(), name_hash, []( const std::shared_ptr< NodeWrapper >& lhs, const BjSON::NameHash& rhs )
	{
		return lhs->m_nameHash < rhs;
	} );
}

ScriptRunner::ScriptRunner( std::shared_ptr< Script > script, ScriptContext& ctx )
	: m_script( script )
	, m_context( ctx )
{
	STRONG_ASSERT( script );
}

bool ScriptRunner::Run()
{
	if ( m_script->m_entryPoint == 0 )
	{
		LOG_ASSERT( m_script->m_nodes.empty(), "Script has nodes, but none are selected as the entry point" );
		return true;
	}

	auto iter = m_script->GetNode( m_script->m_entryPoint );
	if ( !WEAK_ASSERT( iter != m_script->m_nodes.end() && ( *iter )->m_nameHash == m_script->m_entryPoint ) )
		return false;

	if ( !WEAK_ASSERT( m_currentNode = *iter ) )
		return false;

	while ( m_currentNode )
	{
		u32 input_count;
		const BjSON::NameHash* const input_names = m_currentNode->m_node->GetInputs().GetPinNameHashes( input_count );

		for ( u32 idx = 0; idx < input_count; ++idx )
		{
			if ( auto node = m_currentNode->m_inputLinks[ idx ].m_sourceNode.lock() )
			{
				void* const src = node->m_node->GetOutputs().GetPinData( m_currentNode->m_inputLinks[ idx ].m_sourcePin );

				const size_t* src_type = node->m_node->GetOutputs().GetPinTypeID( m_currentNode->m_inputLinks[ idx ].m_sourcePin );
				const size_t* dst_type = m_currentNode->m_node->GetInputs().GetPinTypeID( input_names[ idx ] );

				if ( !WEAK_ASSERT( src_type && dst_type && *src_type == *dst_type ) )
					return false;
				
				m_currentNode->m_node->GetInputs().CopyPinData( input_names[ idx ], src );
			}
			else if ( m_currentNode->m_inputLinks[ idx ].m_sourceNodeNameHash == "__entryPoint"_name )
			{
				if ( ScriptContext::IPin* pin = m_context.GetInput( m_currentNode->m_inputLinks[ idx ].m_sourcePin ) )
				{
					void* const src = pin->GetData();
					size_t src_type = pin->GetTypeID();

					const size_t* dst_type = m_currentNode->m_node->GetInputs().GetPinTypeID( input_names[ idx ] );

					if ( !WEAK_ASSERT( dst_type && src_type == *dst_type ) )
						return false;

					m_currentNode->m_node->GetInputs().CopyPinData( input_names[ idx ], src );
				}
			}
		}

		const u32 result = m_currentNode->m_node->Exec( m_context );
		if ( !WEAK_ASSERT( result != ~0u, "Script failed on {}", m_currentNode->m_name ) )
		{
			m_currentNode = nullptr;
			return false;
		}

		m_currentNode = m_currentNode->m_execLinks[ result ].m_nextNode.lock();
	}

	return true;
}

ScriptEditor::ScriptEditor()
{
	m_nodeEditorContext = ImNodes::CreateContext();
}

ScriptEditor::~ScriptEditor()
{
	ImNodes::DestroyContext( m_nodeEditorContext );
}

std::string ScriptEditor::GetWindowTitle() const
{
	return std::format( "Script Editor: {}###{}", m_scriptPath, (u64)this );
}

void ScriptEditor::Run( IFrameContext& frame_context )
{
	if ( ImGui::Begin( GetWindowTitle().c_str(), &m_open, ImGuiWindowFlags_MenuBar ) )
	{
		ImGui::SetWindowSize( { 1000, 600 }, ImGuiCond_Once );

		if ( ImGui::BeginMenuBar() )
		{
			if ( ImGui::BeginMenu( "Edit" ) )
			{
				if ( ImGui::Selectable( "Select Entry Node" ) )
					m_selectEntryNode = true;

				ImGui::EndMenu();
			}

			if ( ImGui::BeginMenu( "View" ) )
			{
				ImGui::Checkbox( "Show Minimap", &m_showMiniMap );
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		ImNodes::SetCurrentContext( m_nodeEditorContext );

		ImNodes::PushColorStyle( ImNodesCol_NodeBackground, 0x88444444 );
		ImNodes::PushColorStyle( ImNodesCol_NodeBackgroundHovered, 0xff444444 );
		ImNodes::PushColorStyle( ImNodesCol_NodeBackgroundSelected, 0xff222222 );
		ImNodes::PushColorStyle( ImNodesCol_TitleBar, 0xff000000 );
		ImNodes::PushColorStyle( ImNodesCol_TitleBarHovered, 0xff222222 );
		ImNodes::PushColorStyle( ImNodesCol_TitleBarSelected, 0xff111111 );
		ImNodes::BeginNodeEditor();

		{// do the entry point node
			ImNodes::PushColorStyle( ImNodesCol_TitleBar, 0xff001100 );
			ImNodes::PushColorStyle( ImNodesCol_TitleBarHovered, 0xff224422 );
			ImNodes::PushColorStyle( ImNodesCol_TitleBarSelected, 0xff002200 );

			ImNodes::SetNodeGridSpacePos( "__entryPoint"_name, {} );
			ImNodes::BeginNode( "__entryPoint"_name );

			ImNodes::BeginNodeTitleBar();
			ImGui::TextUnformatted( "EntryPoint" );
			ImNodes::EndNodeTitleBar();

			ImNodes::PushColorStyle( ImNodesCol_Pin, 0xffffffff );
			ImNodes::BeginOutputAttribute( GetPinID( "__entryPoint"_name, "__firstNode"_name ), ImNodesPinShape_TriangleFilled );
			ImGui::TextUnformatted( ">>" );
			ImNodes::EndOutputAttribute();
			ImNodes::PopColorStyle();

			ImNodes::PushColorStyle( ImNodesCol_Pin, 0xffff0000 );

			auto iter_to_erase = m_script->m_inputs.end();
			for ( auto iter = m_script->m_inputs.begin(); iter != m_script->m_inputs.end(); ++iter )
			{
				auto& [name, type] = *iter;

				ImNodes::BeginOutputAttribute( GetPinID( "__entryPoint"_name, BjSON::HashName( name.c_str() ) ) );

				if ( ImGui::Button( std::format( "[-]##{}", name ).c_str() ) )
				{
					for ( auto& node : m_script->m_nodes )
					for ( auto& link : node->m_inputLinks )
					if ( link.m_sourceNodeNameHash == "__entryPoint"_name && link.m_sourcePin == BjSON::HashName( name.c_str() ) )
					{
						link.m_sourceNode.reset();
						link.m_sourceNodeNameHash = 0;
						link.m_sourcePin = 0;
					}

					iter_to_erase = iter;
				}

				ImGui::SameLine();
				ImGui::TextUnformatted( name.c_str() );

				ImNodes::EndOutputAttribute();
			}

			if ( iter_to_erase != m_script->m_inputs.end() )
				m_script->m_inputs.erase( iter_to_erase );

			ImNodes::BeginOutputAttribute( GetPinID( "__entryPoint"_name, "__newInput"_name ) );
			ImGui::TextUnformatted( "New Input" );
			ImNodes::EndOutputAttribute();

			ImNodes::PopColorStyle();

			ImNodes::PopColorStyle();
			ImNodes::PopColorStyle();
			ImNodes::PopColorStyle();

			ImNodes::EndNode();

			if ( m_script->m_entryPoint )
			{
				ImNodes::PushColorStyle( ImNodesCol_Link, 0xffffffff );
				ImNodes::PushColorStyle( ImNodesCol_LinkSelected, 0xff888888 );

				ImNodes::Link( GetPinID( "__entryPoint"_name, "__firstNode"_name ), GetPinID( "__entryPoint"_name, "__firstNode"_name ), m_script->m_entryPoint );

				ImNodes::PopColorStyle();
				ImNodes::PopColorStyle();
			}
		}

		for ( const auto node : m_script->m_nodes )
		{
			const i32 node_uid = node->m_nameHash;

			ImNodes::BeginNode( node_uid );

			if ( node->m_needsToSetPosition || m_needsToSetAllNodePositions )
			{
				ImNodes::SetNodeGridSpacePos( node_uid, node->m_position );
				node->m_needsToSetPosition = false;
			}
			else
			{
				node->m_position = ImNodes::GetNodeGridSpacePos( node_uid );
			}

			const f32 width = ImNodes::GetNodeDimensions( node_uid ).x;

			ImNodes::BeginNodeTitleBar();
			ImGui::TextUnformatted( node->m_name.c_str(), strstr( node->m_name.c_str(), "##" ) );
			ImNodes::EndNodeTitleBar();

			u32 exec_pin_count;
			u32 input_pin_count;
			u32 output_pin_count;
			const char* const* const exec_pin_names = node->m_node->GetExecPinNames( exec_pin_count );
			const char* const* const input_pin_names = node->m_node->GetInputs().GetPinNames( input_pin_count );
			const char* const* const output_pin_names = node->m_node->GetOutputs().GetPinNames( output_pin_count );

			f32 max_left_width = ImGui::CalcTextSize( "" ).x;
			f32 max_right_width = ImGui::CalcTextSize( "" ).x;

			for ( u32 index = 0; index < input_pin_count; ++index )
				max_left_width = std::max( max_left_width, ImGui::CalcTextSize( input_pin_names[ index ] ).x );

			for ( u32 index = 0; index < exec_pin_count; ++index )
				max_right_width = std::max( max_right_width, ImGui::CalcTextSize( exec_pin_names[ index ] ).x );

			for ( u32 index = 0; index < output_pin_count; ++index )
				max_right_width = std::max( max_right_width, ImGui::CalcTextSize( output_pin_names[ index ] ).x );

			max_left_width += ImGui::CalcTextSize(" | ").x;

			const u32 max_rows = std::max( input_pin_count + 1, output_pin_count + exec_pin_count );
			const u32 min_rows = std::min( input_pin_count + 1, output_pin_count + exec_pin_count );

			for ( u32 row = 0; row < max_rows; ++row )
			{
				const bool input_exec_pin = row == 0;
				const u32 input_data_pin = row - 1;
				const u32 output_exec_pin = row;
				const u32 output_data_pin = row - exec_pin_count;

				f32 remaining_width = max_left_width + max_right_width;

				if ( input_exec_pin )
				{
					ImNodes::PushColorStyle( ImNodesCol_Pin, 0xffffffff );
					ImNodes::BeginInputAttribute( node_uid, ImNodesPinShape_TriangleFilled );

					remaining_width -= ImGui::CalcTextSize( "" ).x;
					ImGui::TextUnformatted( "" );

					ImNodes::EndInputAttribute();
					ImNodes::PopColorStyle();

					if ( row < min_rows )
						ImGui::SameLine();
				}
				else if ( input_data_pin < input_pin_count )
				{
					const char* const pin_name = input_pin_names[ input_data_pin ];

					if ( strcmp( pin_name, "__ignore" ) )
					{
						const int id = GetPinID( node->m_nameHash, BjSON::HashName( pin_name ) );

						ImNodes::PushColorStyle( ImNodesCol_Pin, 0xffff0000 );
						ImNodes::BeginInputAttribute( id, ImNodesPinShape_CircleFilled );

						remaining_width -= ImGui::CalcTextSize( pin_name ).x;
						ImGui::TextUnformatted( pin_name );

						ImNodes::EndInputAttribute();
						ImNodes::PopColorStyle();

						if ( row < min_rows )
							ImGui::SameLine();
					}
				}

				if ( output_exec_pin < exec_pin_count )
				{
					const char* const pin_name = exec_pin_names[ output_exec_pin ];
					
					if ( strcmp( pin_name, "__ignore" ) )
					{
						const int id = GetPinID( node->m_nameHash, BjSON::HashName( pin_name ) );

						ImNodes::PushColorStyle( ImNodesCol_Pin, 0xffffffff );
						ImNodes::BeginOutputAttribute( id, ImNodesPinShape_TriangleFilled );

						remaining_width -= ImGui::CalcTextSize( pin_name ).x;
						ImGui::Indent( remaining_width );
						ImGui::TextUnformatted( pin_name );

						ImNodes::EndOutputAttribute();
						ImNodes::PopColorStyle();
					}
				}
				else if ( output_data_pin < output_pin_count )
				{
					const char* const pin_name = output_pin_names[ output_data_pin ];

					if ( strcmp( pin_name, "__ignore" ) )
					{
						const int id = GetPinID( node->m_nameHash, BjSON::HashName( pin_name ) );

						ImNodes::PushColorStyle( ImNodesCol_Pin, 0xffff0000 );
						ImNodes::BeginOutputAttribute( id, ImNodesPinShape_CircleFilled );

						remaining_width -= ImGui::CalcTextSize( pin_name ).x;
						ImGui::Indent( remaining_width );
						ImGui::TextUnformatted( pin_name );

						ImNodes::EndOutputAttribute();
						ImNodes::PopColorStyle();
					}
				}
			}

			// ImGui::NewLine();
			node->m_node->DoCustomUI();

			ImNodes::EndNode();

			for ( u32 row = 0; row < max_rows; ++row )
			{
				const bool input_exec_pin = row == 0;
				const u32 input_data_pin = row - 1;
				const u32 output_exec_pin = row;
				const u32 output_data_pin = row - exec_pin_count;

				ImNodes::PushColorStyle( ImNodesCol_Link, 0xffff0000 );
				ImNodes::PushColorStyle( ImNodesCol_LinkSelected, 0xff880000 );

				if ( input_data_pin < input_pin_count )
				{
					const char* const pin_name = input_pin_names[ input_data_pin ];
					const int dest_pin_id = GetPinID( node->m_nameHash, BjSON::HashName( pin_name ) );
					const char* dest_pin_type = node->m_node->GetInputs().GetPinTypeName( BjSON::HashName( pin_name ) );

					Script::DataLink& link = node->m_inputLinks[ input_data_pin ];
					if ( auto source_node = link.m_sourceNode.lock() )
					{
						ImNodes::Link( dest_pin_id, GetPinID( source_node->m_nameHash, link.m_sourcePin ), dest_pin_id );
					}
					else if ( link.m_sourceNodeNameHash == "__entryPoint"_name )
					{
						ImNodes::Link( dest_pin_id, GetPinID( "__entryPoint"_name, link.m_sourcePin ), dest_pin_id );

						for ( auto& [name, type] : m_script->m_inputs )
						{
							if ( type == 0 && link.m_sourcePin == BjSON::HashName( name.c_str() ) )
							{
								if ( auto t = node->m_node->GetInputs().GetPinTypeID( BjSON::HashName( pin_name ) ) )
								{
									type = *t;
									break;
								}
							}
						}
					}
					else
					{
						link.m_sourceNode.reset();
						link.m_sourceNodeNameHash = 0;
						link.m_sourcePin = 0;
					}
				}

				ImNodes::PopColorStyle();
				ImNodes::PopColorStyle();

				if ( output_exec_pin < exec_pin_count )
				{
					const char* const pin_name = exec_pin_names[ output_exec_pin ];
					const int source_pin_id = GetPinID( node->m_nameHash, BjSON::HashName( pin_name ) );

					Script::ExecLink& link = node->m_execLinks[ output_exec_pin ];
					if ( link.m_nextNode.lock() )
					{
						ImNodes::PushColorStyle( ImNodesCol_Link, 0xffffffff );
						ImNodes::PushColorStyle( ImNodesCol_LinkSelected, 0xff888888 );

						if ( auto next_node = link.m_nextNode.lock() )
							if ( int dest_node_id = next_node->m_nameHash )
								ImNodes::Link( source_pin_id, source_pin_id, dest_node_id );

						ImNodes::PopColorStyle();
						ImNodes::PopColorStyle();
					}
					else
					{
						link.m_nextNode.reset();
						link.m_nextNodeNameHash = 0;
					}
				}
			}
		}

		if ( m_showMiniMap )
			ImNodes::MiniMap();

		ImNodes::EndNodeEditor();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();

		static ImVec2 new_node_pos = {};
		if ( ImGui::IsMouseReleased( ImGuiMouseButton_Right ) && ImNodes::IsEditorHovered() )
		{
			ImGui::OpenPopup( "Add Node Menu" );
			new_node_pos.x = ImGui::GetMousePos().x - ImNodes::EditorContextGetPanning().x - ImGui::GetWindowPos().x;
			new_node_pos.y = ImGui::GetMousePos().y - ImNodes::EditorContextGetPanning().y - ImGui::GetWindowPos().y;
		}

		if ( ImGui::BeginPopup( "Add Node Menu" ) )
		{
			if ( m_newScriptNode = ScriptNodes::DoCreateScriptNodeCombo() )
			{
				std::string new_node_name = m_newScriptNode->GetScriptNodeTypeName();
				std::string new_node_name_with_suffix = new_node_name;
				int new_node_name_suffix = 0;

				bool new_node_name_is_unique;
				do
				{
					new_node_name_is_unique = true;
					for ( auto& node : m_script->m_nodes )
					{
						if ( node->m_name == new_node_name_with_suffix )
						{
							new_node_name_is_unique = false;
							break;
						}
					}

					if ( !new_node_name_is_unique )
					{
						new_node_name_suffix += 1;
						new_node_name_with_suffix = std::format( "{}##{}", new_node_name, new_node_name_suffix );
					}
				}
				while ( !new_node_name_is_unique );

				strcpy_s( m_newNodeName, new_node_name_with_suffix.c_str() );

				u32 input_count;
				u32 exec_count;
				m_newScriptNode->GetInputs().GetPinNames( input_count );
				m_newScriptNode->GetExecPinNames( exec_count );

				Script::NodeWrapper new_node( m_newNodeName );
				new_node.m_node = std::move( m_newScriptNode );
				new_node.m_execLinks.resize( exec_count );
				new_node.m_inputLinks.resize( input_count );
				new_node.m_position = new_node_pos;

				auto iter = m_script->GetNode( new_node.m_nameHash );

				WEAK_ASSERT( iter == m_script->m_nodes.end() || ( *iter )->m_name != new_node.m_name, "A node already exists called {}", new_node.m_name );

				m_script->m_nodes.insert( iter, std::make_shared< Script::NodeWrapper >( std::move( new_node ) ) );
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if ( int start_pin_id, end_pin_id; ImNodes::IsLinkCreated( &start_pin_id, &end_pin_id ) )
		{
			if ( start_pin_id == GetPinID( "__entryPoint"_name, "__firstNode"_name ) )
			{
				m_script->m_entryPoint = end_pin_id;
			}
			else if ( start_pin_id == GetPinID( "__entryPoint"_name, "__newInput"_name ) )
			{
				if ( auto end_pin = WEAK_ASSERT( GetPinFromID( end_pin_id ) ) )
				if ( auto end_node = m_script->GetNode( end_pin->first ); WEAK_ASSERT( end_node != m_script->m_nodes.end() && ( *end_node )->m_nameHash == end_pin->first ) )
				if ( auto end_pin_type = WEAK_ASSERT( ( *end_node )->m_node->GetInputs().GetPinTypeID( end_pin->second ) ) )
				{
					m_newInputType = *end_pin_type;
					m_newInputDestPinID = end_pin_id;
					m_newInputName.clear();

					if ( const u32 end_pin_index = ( *end_node )->m_node->GetInputs().GetPinIndex( end_pin->second ); end_pin_index != ~0u )
					{
						u32 pin_count;
						const char* const end_pin_name = ( *end_node )->m_node->GetInputs().GetPinNames( pin_count )[ end_pin_index ];
						if ( end_pin_index < pin_count && end_pin_name )
						{
							m_newInputName = end_pin_name;
						}
					}


					ImGui::OpenPopup( "New Input Node" );
				}
			}
			else if ( auto start_pin = GetPinFromID( start_pin_id ) )
			{
				const auto& [start_node_name_hash, start_pin_name_hash] = *start_pin;

				if ( start_node_name_hash == "__entryPoint"_name )
				{
					if ( auto end_pin = GetPinFromID( end_pin_id ) )
					if ( auto end_node = m_script->GetNode( end_pin->first ); end_node != m_script->m_nodes.end() && ( *end_node )->m_nameHash == end_pin->first )
					if ( u32 index = ( *end_node )->m_node->GetInputs().GetPinIndex( end_pin->second ); index != ~0u )
					{
						( *end_node )->m_inputLinks[ index ].m_sourceNodeNameHash = start_node_name_hash;
						( *end_node )->m_inputLinks[ index ].m_sourcePin = start_pin_name_hash;
						( *end_node )->m_inputLinks[ index ].m_sourceNode.reset();
					}
				}
				if ( auto _start_node = m_script->GetNode( start_node_name_hash ); _start_node != m_script->m_nodes.end() && ( *_start_node )->m_nameHash == start_node_name_hash )
				{
					auto& start_node = *_start_node;

					// data links have a valid end pin id
					if ( auto end_pin = GetPinFromID( end_pin_id ) )
					{
						const auto& [end_node_name_hash, end_pin_name_hash] = *end_pin;

						if ( auto _end_node = m_script->GetNode( end_node_name_hash ); _end_node != m_script->m_nodes.end() )
						{
							auto& end_node = *_end_node;

							const char* end_type_name = end_node->m_node->GetInputs().GetPinTypeName( end_pin_name_hash );
							const char* start_type_name = start_node->m_node->GetOutputs().GetPinTypeName( start_pin_name_hash );

							const size_t* start_type = start_node->m_node->GetOutputs().GetPinTypeID( start_pin_name_hash );
							const size_t* end_type = end_node->m_node->GetInputs().GetPinTypeID( end_pin_name_hash );

							if ( start_type && end_type && *start_type == *end_type )
							{
								const u32 end_pin_index = end_node->m_node->GetInputs().GetPinIndex( end_pin_name_hash );

								if ( end_pin_index != ~0u )
								{
									Script::DataLink& link = end_node->m_inputLinks[ end_pin_index ];
									link.m_sourcePin = start_pin_name_hash;
									link.m_sourceNode = start_node;
									link.m_sourceNodeNameHash = start_node->m_nameHash;
								}
							}
						}
					}
					// exec links have a valid end node id rather than an end pin id
					else if ( auto end_node = m_script->GetNode( end_pin_id ); end_node != m_script->m_nodes.end() )
					{
						if ( auto _start_node = m_script->GetNode( start_node_name_hash ); _start_node != m_script->m_nodes.end() )
						{
							auto& start_node = *_start_node;

							if ( const u32 exec_pin_index = start_node->m_node->GetExecPinIndex( start_pin_name_hash ); exec_pin_index != ~0u )
							{
								Script::ExecLink& link = start_node->m_execLinks[ exec_pin_index ];
								link.m_nextNodeNameHash = ( *end_node )->m_nameHash;
								link.m_nextNode = *end_node;
							}
						}
					}
				}
			}
		}

		if ( ImGui::BeginPopup( "New Input Node" ) )
		{
			ImGui::InputText( "Name", &m_newInputName );

			if ( ImGui::Button( "Cancel" ) )
			{
				m_newInputName.clear();
				m_newInputType = 0;
				m_newInputDestPinID = 0;

				ImGui::CloseCurrentPopup();
			}
			
			ImGui::SameLine();

			if ( ImGui::Button( "Ok" ) )
			{
				m_script->m_inputs.insert( { m_newInputName, m_newInputType } );
				
				if ( auto dest_pin = GetPinFromID( m_newInputDestPinID ) )
				if ( auto dest_node = m_script->GetNode( dest_pin->first ); dest_node != m_script->m_nodes.end() && ( *dest_node )->m_nameHash == dest_pin->first )
				if ( u32 dest_pin_idx = ( *dest_node )->m_node->GetInputs().GetPinIndex( dest_pin->second ); dest_pin_idx != ~0u )
				{
					( *dest_node )->m_inputLinks[ dest_pin_idx ].m_sourceNodeNameHash = "__entryPoint"_name;
					( *dest_node )->m_inputLinks[ dest_pin_idx ].m_sourcePin = BjSON::HashName( m_newInputName.c_str() );
					( *dest_node )->m_inputLinks[ dest_pin_idx ].m_sourceNode.reset();
				}

				m_newInputName.clear();
				m_newInputType = 0;
				m_newInputDestPinID = 0;

				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if ( LowLevel::GetInput().GetButtonState( onyx::InputAxis::Keyboard_Delete ) == ButtonState::Pressed )
		{
			std::vector< i32 > selected_ids;

			selected_ids.resize( ImNodes::NumSelectedNodes() );
			if ( !selected_ids.empty() )
				ImNodes::GetSelectedNodes( selected_ids.data() );

			for ( const i32& node_id : selected_ids )
				if ( const auto iter = m_script->GetNode( node_id ); iter != m_script->m_nodes.end() )
					m_script->m_nodes.erase( iter );

			selected_ids.resize( ImNodes::NumSelectedLinks() );
			if ( !selected_ids.empty() )
				ImNodes::GetSelectedLinks( selected_ids.data() );

			for ( const i32& link_id : selected_ids )
			{
				if ( const auto unique_pin = GetPinFromID( link_id ) )
				{
					const auto& [node_id, pin_id] = *unique_pin;

					if ( const auto node = m_script->GetNode( node_id ); node != m_script->m_nodes.end() && ( *node )->m_nameHash == node_id )
					{
						if ( const u32 index = ( *node )->m_node->GetInputs().GetPinIndex( pin_id ); index != ~0u )
							( *node )->m_inputLinks[ index ] = Script::DataLink();
						else if ( const u32 index = ( *node )->m_node->GetExecPinIndex( pin_id ); index != ~0u )
							( *node )->m_execLinks[ index ] = Script::ExecLink();
					}
					else if ( node_id == "__entryPoint"_name )
					{
						if ( pin_id == "__firstNode"_name )
							m_script->m_entryPoint = 0;
					}
				}
			}
		}

		if ( ImNodes::NumSelectedNodes() > 0 )
		{
			ImVec2 movement = {};

			movement.x += 
				f32( onyx::LowLevel::GetInput().GetButtonState( onyx::InputAxis::Keyboard_RightArrow ) == ButtonState::Pressed ) -
				f32( onyx::LowLevel::GetInput().GetButtonState( onyx::InputAxis::Keyboard_LeftArrow ) == ButtonState::Pressed );

			movement.y +=
				f32( onyx::LowLevel::GetInput().GetButtonState( onyx::InputAxis::Keyboard_DownArrow ) == ButtonState::Pressed ) -
				f32( onyx::LowLevel::GetInput().GetButtonState( onyx::InputAxis::Keyboard_UpArrow ) == ButtonState::Pressed );

			if ( movement.x != 0.f || movement.y != 0.f )
			{
				std::vector< i32 > selected_node_ids; 
				selected_node_ids.resize( ImNodes::NumSelectedNodes() );

				ImNodes::GetSelectedNodes( selected_node_ids.data() );

				for ( const i32 node_id : selected_node_ids )
				if ( auto iter = m_script->GetNode( node_id ); iter != m_script->m_nodes.end() )
				{
					( *iter )->m_position.x += movement.x;
					( *iter )->m_position.y += movement.y;
					( *iter )->m_needsToSetPosition = true;
				}
			}
		}

		if ( m_selectEntryNode && ImNodes::NumSelectedNodes() == 1 )
		{
			i32 selected_node_id;
			ImNodes::GetSelectedNodes( &selected_node_id );

			if ( auto entry_point = m_script->GetNode( selected_node_id ); entry_point != m_script->m_nodes.end() )
				m_script->m_entryPoint = ( *entry_point )->m_nameHash;

			m_selectEntryNode = false;
		}

		if ( i32 pin_id; ImNodes::IsPinHovered( &pin_id ) || ImNodes::IsLinkHovered( &pin_id ) )
		{
			if ( auto pin = GetPinFromID( pin_id ) )
			{
				if ( auto node = m_script->GetNode( pin->first ); node != m_script->m_nodes.end() )
				{
					const char* pin_type_name = ( *node )->m_node->GetOutputs().GetPinTypeName( pin->second );
					if ( !pin_type_name )
						pin_type_name = ( *node )->m_node->GetInputs().GetPinTypeName( pin->second );

					if ( pin_type_name )
						ImGui::SetTooltip( "%s", pin_type_name );
				}
			}
		}

		ImNodes::SetCurrentContext( nullptr );
	}

	ImGui::End();

	m_needsToSetAllNodePositions = false;
}

i32 ScriptEditor::GetPinID( BjSON::NameHash node_name, BjSON::NameHash pin_name )
{
	if ( node_name == 0 || pin_name == 0 )
		return 0;

	const i32 result = node_name ^ pin_name;

	const std::pair< BjSON::NameHash, BjSON::NameHash > pair { node_name, pin_name };
	const auto insert_result = m_uidToPin.insert( { result, pair } );

	WEAK_ASSERT( insert_result.second || insert_result.first->second == pair, "Pin/Pin ID collision!" );

	return result;
}

const std::pair< BjSON::NameHash, BjSON::NameHash >* ScriptEditor::GetPinFromID( i32 uid )
{
	const auto iter = m_uidToPin.find( uid );

	if ( iter == m_uidToPin.end() )
		return nullptr;

	return &iter->second;
}

}
