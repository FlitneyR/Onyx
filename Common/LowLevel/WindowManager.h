#pragma once

#include "glm/glm.hpp"
#include <memory>

#include "Common/Window.h"

namespace onyx
{

struct IWindowManager
{
	struct CreateWindowArgs
	{
		glm::ivec2 size { 1280, 720 };
		glm::ivec2 pos { 50, 50 };
		const char* title = "Onyx";
		bool fullscreen : 1 = false;
		bool resizable : 1 = false;
	};

	virtual std::shared_ptr< IWindow > CreateWindow( const CreateWindowArgs& args ) = 0;
	virtual bool WantsToQuit() const = 0;
	virtual void ProcessEvents() = 0;

	virtual void ImGuiNewFrame() {}
};

}
