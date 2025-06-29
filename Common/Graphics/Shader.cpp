#include "Shader.h"

#include <fstream>
#include "glslang/Public/ShaderLang.h"
#include "glslang/Public/ResourceLimits.h"
#include "glslang/SPIRV/GlslangToSpv.h"

#include "imgui_stdlib.h"

namespace onyx
{

bool ShaderAsset::Import( const char* filename )
{
	std::ifstream file( filename, std::ios::ate );
	if ( !WEAK_ASSERT( file.is_open(), "Failed to open file: {}", filename ) )
		return false;
	
	const u32 file_size = file.tellg();
	m_source.resize( file_size );

	file.seekg( std::ios::beg )
		.read( m_source.data(), file_size );

	return true;
}

struct GLSLangDeleter : DeleteQueue::IDeleter
{
	void Execute() override { glslang::FinalizeProcess(); }
};

bool ShaderAsset::CompileSource( LowLevel::Config::GraphicsAPI api, ShaderStage stage, std::string& log )
{
	log.clear();

	switch ( api )
	{
	case LowLevel::Config::GraphicsAPI::Vulkan:
	{
		static bool s_hasInitialisedGlslang = false;
		if ( !s_hasInitialisedGlslang )
		{
			glslang::InitializeProcess();
			LowLevel::GetGraphicsContext().m_shutdownDeleteQueue.Add< GLSLangDeleter >();
		}
		
		EShLanguage language = EShLangCount;
		switch ( stage )
		{
		case ShaderStage::Compute: language = EShLangCompute; break;
		case ShaderStage::Vertex: language = EShLangVertex; break;
		case ShaderStage::Fragment: language = EShLangFragment; break;
		default: break;
		}

		if ( !WEAK_ASSERT( language < EShLangCount, "Shader stage couldn't be determined" ) )
			return false;

		glslang::TShader shader( language );

		const char* const source = m_source.c_str();
		shader.setStrings( &source, 1 );
		shader.setEnvInput( glslang::EShSourceGlsl, language, glslang::EShClientVulkan, 450 );
		shader.setEnvClient( glslang::EShClientVulkan, glslang::EShTargetClientVersion::EShTargetVulkan_1_3 );
		shader.setEnvTarget( glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_6 );


		if ( !LOG_ASSERT( shader.parse( GetDefaultResources(), 450, true, EShMsgDefault /*todo: , includer*/ ), "Failed to compile a shader:\n{}", log += shader.getInfoLog() ) )
			return false;

		log += std::string( shader.getInfoLog() );
		glslang::TProgram program;
		program.addShader( &shader );
		program.getIntermediate( language );

		if ( !LOG_ASSERT( program.link( EShMsgDefault ), "Failed to compile a program: \n{}", log += program.getInfoLog() ) )
			return false;

		log += std::string( shader.getInfoLog() );
		m_byteCode.clear();
		glslang::GlslangToSpv( *STRONG_ASSERT( program.getIntermediate( language ) ), m_byteCode );

		//std::stringstream strstr;
		//for ( u32& word : m_byteCode )
		//	strstr << word << " ";

		//INFO( "byte code: {}", strstr.str() );

		return true;
	}

	default:
		return false;
	}
}


void ShaderAsset::Load( LoadType type )
{
	m_loadingState = LoadingState::Loading;

	const BjSON::IReadOnlyObject* reader = GetReader();
	if ( !WEAK_ASSERT( reader ) )
	{
		m_loadingState = LoadingState::Errored;
		return;
	}

	if ( !WEAK_ASSERT( reader->GetLiteral< u32 >( "__assetType"_name ) == "Shader"_name ) )
	{
		m_loadingState = LoadingState::Errored;
		return;
	}

	if ( type == LoadType::Editor )
	{
		m_source.resize( reader->GetLiteral( "Source"_name ) );
		reader->GetLiteral( "Source"_name, m_source.data(), m_source.size() );
	}

	m_byteCode.resize( reader->GetLiteral( "ByteCode"_name ) / sizeof( u32 ) );
	reader->GetLiteral( "ByteCode"_name, m_byteCode.data(), m_byteCode.size() * sizeof( u32 ) );

	//std::stringstream strstr;
	//for ( u32& word : m_byteCode )
	//	strstr << word << " ";

	//INFO( "byte code: {}", strstr.str() );

	m_loadingState = LoadingState::Loaded;
}

void ShaderAsset::Save( BjSON::IReadWriteObject& writer, SaveType type )
{
	writer.SetLiteral( "__assetType"_name, "Shader"_name );
	
	if ( type != SaveType::Export )
		writer.SetLiteral( "Source"_name, m_source );

	writer.SetLiteral( "ByteCode"_name, m_byteCode.data(), m_byteCode.size() * sizeof( m_byteCode[ 0 ] ) );

	//std::stringstream strstr;
	//for ( u32& word : m_byteCode )
	//	strstr << word << " ";

	//INFO( "byte code: {}", strstr.str() );
}

void ShaderAsset::DoAssetManagerButton( const char* name, const char* path, f32 width, std::shared_ptr< IAsset > asset, IFrameContext& frame_context )
{
	switch ( m_loadingState )
	{
	case LoadingState::Loaded:
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.25f, 0.25f, 0.f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.25f, 0.5f, 0.f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.125f, 0.125f, 0.f, 1.f ) );
		break;
	case LoadingState::Errored:
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.25f, 0.f, 0.f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.5f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.125f, 0.f, 0.f, 1.f ) );
		break;
	default:
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.25f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.25f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.125f, 0.125f, 0.125f, 1.f ) );
		break;
	}

	if ( ImGui::Button( name ) )
	{
		if ( m_loadingState == LoadingState::Unloaded )
			Load( IAsset::LoadType::Editor );

		if ( m_loadingState == LoadingState::Loaded )
		{
			ShaderEditorWindow* const window = onyx::editor::AddWindow< ShaderEditorWindow >();
			window->m_shader = std::static_pointer_cast< ShaderAsset >( asset );
		}
	}

	ImGui::PopStyleColor( 3 );
}

void ShaderEditorWindow::Run( IFrameContext& frame_context )
{
	if ( ImGui::Begin( GetWindowTitle().c_str(), &m_open, ImGuiWindowFlags_MenuBar ) )
	{
		if ( ImGui::BeginMenuBar() )
		{
			if ( ImGui::BeginMenu( "File" ) )
			{
				if ( ImGui::MenuItem( "Import", nullptr, nullptr, m_shader != nullptr ) )
				{
					const std::string file_path = LowLevel::GetWindowManager().DoOpenFileDialog();
					if ( !file_path.empty() )
						m_shader->Import( file_path.c_str() );
				}

				ImGui::EndMenu();
			}

			if ( ImGui::BeginMenu( "Compile" ) )
			{
				ImGui::Combo( "Graphics API", (int*)&m_compileGraphicsAPI, LowLevel::Config::s_GraphicsAPINames, (int)LowLevel::Config::GraphicsAPI::Count );
				ImGui::Combo( "Shader Stage", (int*)&m_compileShaderStage, ShaderAsset::s_ShaderStageNames, (int)ShaderAsset::ShaderStage::Count );

				if ( ImGui::MenuItem( "Compile", nullptr, nullptr, m_shader != nullptr ) )
				{
					const bool success = m_shader->CompileSource( m_compileGraphicsAPI, m_compileShaderStage, m_log );
					m_log = std::format( "{}\n{}", success ? "Success" : "Failed", m_log );
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		ImVec2 text_editor_size = ImGui::GetWindowSize();
		text_editor_size.x -= 10.f;
		text_editor_size.y *= 0.7f;

		if ( m_shader )
			ImGui::InputTextMultiline( "##Source Code", &m_shader->m_source, text_editor_size );

		ImVec2 log_size = ImGui::GetWindowSize();
		log_size.x -= 10.f;
		log_size.y *= 0.2f;

		ImGui::InputTextMultiline( "##Log", &m_log, log_size );
	}

	ImGui::End();
}

std::string ShaderEditorWindow::GetWindowTitle() const
{
	return std::format( "Shader Editor: {}###{}", m_shader ? m_shader->m_path : "", (u64)this );
}

}
