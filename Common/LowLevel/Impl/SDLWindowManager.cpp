#include "SDLWindowManager.h"

#include "Common/Graphics/GraphicsContext.h"
#include "Common/LowLevel/LowLevelInterface.h"
#include "Common/LowLevel/Impl/SDLInput.h"

#include "SDL2/SDL_vulkan.h"
#include "imgui_impl_sdl2.h"

namespace onyx
{

SDLWindow::SDLWindow( SDL_Window* window ) : m_window( window )
{
	SDL_SetWindowData( window, "owner", this );
	m_windowContext = LowLevel::GetGraphicsContext().CreateWindowContext( *this );
}

SDLWindow::~SDLWindow()
{
	Close();
}

bool SDLWindow::HasClosed() const
{
	return m_window == nullptr;
}

void SDLWindow::Close()
{
	SDL_DestroyWindow( m_window );
	m_window = nullptr;
}

glm::uvec2 SDLWindow::GetSize() const
{
	ASSERT( !HasClosed() );

	glm::ivec2 result;
	SDL_GetWindowSizeInPixels( m_window, &result.x, &result.y );
	return result;
}

void SDLWindow::SetSize( const glm::uvec2& new_size )
{
	ASSERT( !HasClosed() );

	SDL_SetWindowSize( m_window, new_size.x, new_size.y );
}

std::shared_ptr< IWindow > SDLWindowManager::CreateWindow( const CreateWindowArgs& args )
{
	if ( !ASSERT( !LowLevel::GetConfig().enableImGui || m_windows.empty(), "ImGui does not support multiple windows" ) )
		return nullptr;

	u32 flags = 0;

	switch ( LowLevel::GetConfig().graphicsAPI )
	{
	case LowLevel::Config::GraphicsAPI::Vulkan: flags |= SDL_WINDOW_VULKAN; break;
	default:
		ASSERT( false, "Unsupported graphics API" );
		return {};
	}

	flags |= SDL_WINDOW_FULLSCREEN * args.fullscreen;
	flags |= SDL_WINDOW_RESIZABLE * args.resizable;

	SDL_Window* window = ASSERT( SDL_CreateWindow( args.title, args.pos.s, args.pos.y, args.size.x, args.size.y, flags ),
		"Failed to create SDL window: {}", SDL_GetError() );

	if ( LowLevel::GetConfig().enableImGui )
	{
		switch ( LowLevel::GetConfig().graphicsAPI )
		{
		case LowLevel::Config::GraphicsAPI::Vulkan: ImGui_ImplSDL2_InitForVulkan( window ); break;
		default:
			ASSERT( false, "Unsupported graphics API for ImGui" );
			break;
		}
	}

	std::shared_ptr< SDLWindow > i_window = std::make_shared< SDLWindow >( window );
	m_windows.insert( i_window );

	return i_window;
}

void SDLWindowManager::ProcessEvents()
{
	SDL_Event event;

	( (SDLInput&)LowLevel::GetInput() ).ResetMouseState();

	while ( SDL_PollEvent( &event ) )
	{
		switch ( event.type )
		{
		case SDL_QUIT: m_wantsToQuit = true; break;
		case SDL_WINDOWEVENT:
		{
			switch ( event.window.event )
			{
			case SDL_WINDOWEVENT_CLOSE:
				std::erase_if( m_windows, [ id = event.window.windowID ]( const std::shared_ptr< SDLWindow >& window )
				{
					if ( SDL_GetWindowID( window->m_window ) == id )
					{
						window->Close();
						return true;
					}

					return false;
				} );
				break;
			}
		}
		}

		if ( LowLevel::GetConfig().enableImGui )
			ImGui_ImplSDL2_ProcessEvent( &event );

		( (SDLInput&)LowLevel::GetInput() ).ProcessEvent( event );
	}
}

void SDLWindowManager::ImGuiNewFrame()
{
	ImGui_ImplSDL2_NewFrame();
}

bool SDLWindowManager::WantsToQuit() const
{
	return m_wantsToQuit;
}

}
