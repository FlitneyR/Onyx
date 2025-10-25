#pragma once

#include "Onyx/LowLevel/LowLevelInput.h"
#include "SDL2/SDL_events.h"

#include <vector>

namespace onyx
{

struct SDLInput : LowLevelInput
{
	SDLInput();
	~SDLInput();

	void ResetMouseState();
	void ProcessEvent( const SDL_Event& event );

private:
	std::vector< SDL_GameController* > m_controllers;

	void AddController( u8 controller_index );
	void RemoveController( u8 controller_index );

	static InputAxis InputAxisFromKey( SDL_Keycode key );
	static InputAxis InputAxisFromMouseButton( u8 button );
	static InputAxis InputAxisFromControllerButton( u8 button );
	static InputAxis GetInputAxisFromControllerAxis( u8 axis );
};

}
