#include "Log.h"

#include "Common/LowLevel/LowLevelInterface.h"
#include "Common/Graphics/FrameContext.h"
#include "Common/Editor/Window.h"

#include "imgui.h"

#include <glm/gtx/transform.hpp>

#include "Common/ECS/World.h"
#include "Common/ECS/Query.h"
#include "Common/ECS/System.h"
#include "Common/ECS/SystemSet.h"
#include "Common/ECS/CommandBuffer.h"

struct TickSystemContext
{
	onyx::ecs::CommandBuffer cmd;
	f32 deltaTime;
};

struct LifeTime
{
	f32 remaining;
};

void UpdateLifetimes( TickSystemContext& ctx, const onyx::ecs::Query< LifeTime >& entities )
{
	for ( auto& entity : entities )
		if ( ( entity.Get< LifeTime >().remaining -= ctx.deltaTime ) < 0.f )
			ctx.cmd.RemoveEntity( entity.ID() );
}

int main( int argc, const char** argv )
{
	INFO( "Launching Chrono Editor" );

	// todo: load this from a file
	onyx::LowLevel::Config config;
	config.windowManager = onyx::LowLevel::Config::WindowManager::SDL;
	config.graphicsAPI = onyx::LowLevel::Config::GraphicsAPI::Vulkan;
	config.enableImGui = true;

	onyx::LowLevel::Init( config );

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

				onyx::editor::DoWindows();

				graphics_context.EndFrame( *frame_context );
			}
		}
	}

	onyx::LowLevel::CleanUp();

	INFO( "Closing Chrono Editor" );
	return 0;
}
