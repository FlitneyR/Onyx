#pragma once

#include "Onyx/LowLevel/WindowManager.h"
#include "SDL2/SDL.h"

#include <set>

namespace onyx
{

struct SDLWindow : IWindow
{
	SDL_Window* m_window;

	SDLWindow( SDL_Window* window );
	~SDLWindow();

	bool HasClosed() const override;
	void Close() override;
	glm::uvec2 GetSize() const override;
	void SetSize( const glm::uvec2& new_size ) override;
};

struct SDLWindowManager : IWindowManager
{
	std::shared_ptr< IWindow > CreateWindow( const CreateWindowArgs& args ) override;
	bool WantsToQuit() const override;
	void ProcessEvents() override;

	void ImGuiNewFrame() override;

private:
	bool m_wantsToQuit = false;

	std::set< std::shared_ptr< SDLWindow > > m_windows;
};

}
