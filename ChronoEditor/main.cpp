#include "Log.h"

#include "Common/LowLevel/LowLevelInterface.h"
#include "Common/Graphics/FrameContext.h"
#include "Common/Editor/Window.h"

#include "imgui.h"

int main( int argc, const char** argv )
{
	INFO( "Loading Onyx core asset pack" );
	std::ifstream core_asset_pack_file( "../Common/onyx.pack", std::ios::binary | std::ios::beg );
	
	std::unique_ptr< BjSON::Decoder > core_decoder = nullptr;
	std::unique_ptr< onyx::AssetManager > core_asset_manager = nullptr;

	if ( WEAK_ASSERT( core_asset_pack_file.is_open(), "onyx.pack file missing" ) )
	{
		core_decoder = std::make_unique< BjSON::Decoder >( core_asset_pack_file );
		core_asset_manager = std::make_unique< onyx::AssetManager >( core_decoder->GetRootObject() );
	}

	INFO( "Launching Chrono Editor" );

	// todo: load this from a file
	onyx::LowLevel::Config config;
	config.windowManager = onyx::LowLevel::Config::WindowManager::SDL;
	config.graphicsAPI = onyx::LowLevel::Config::GraphicsAPI::Vulkan;
	config.enableImGui = true;

	onyx::LowLevel::Init( config, core_asset_manager.get() );

	onyx::LowLevelInput& input = onyx::LowLevel::GetInput();
	onyx::IWindowManager& window_manager = onyx::LowLevel::GetWindowManager();
	onyx::IGraphicsContext& graphics_context = onyx::LowLevel::GetGraphicsContext();

	onyx::IWindowManager::CreateWindowArgs create_window_args;
	create_window_args.size = { 1920, 1080 };
	create_window_args.title = "Chrono Editor";
	create_window_args.resizable = true;

	if ( std::shared_ptr< onyx::IWindow > editor_window = window_manager.CreateWindow( create_window_args ) )
	{
		while ( !window_manager.WantsToQuit() )
		{
			window_manager.ProcessEvents();
			input.UpdateButtonStates();

			if ( onyx::IFrameContext* frame_context = graphics_context.BeginFrame( *editor_window ) )
			{
				ImGui::DockSpaceOverViewport();

				if ( ImGui::BeginMainMenuBar() )
				{
					onyx::editor::DoWindowsMenu();

					ImGui::EndMainMenuBar();
				}

				onyx::editor::DoWindows( *frame_context );

				graphics_context.EndFrame( *frame_context );
			}
		}
	}

	onyx::LowLevel::CleanUp();

	INFO( "Closing Chrono Editor" );
	return 0;
}
