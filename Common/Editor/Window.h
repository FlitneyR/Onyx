#pragma once

#include "imgui.h"
#include "Common/Graphics/FrameContext.h"

namespace onyx::editor
{

struct IWindow
{
	virtual ~IWindow() = default;

	// override this with the name of your window class
	inline static const char* const s_name = "Unnamed Window";
	virtual const char* GetName() const { return s_name; }
	virtual void Run( IFrameContext& frame_context ) = 0;
	virtual std::string GetWindowTitle() const = 0;

	bool m_open = true;
};

void AddWindow( std::unique_ptr< IWindow > window );

template< typename Window, typename ... Args >
Window* AddWindow( Args ... args )
{
	std::unique_ptr< Window > window = std::make_unique< Window >( args ... );
	Window* result = window.get();
	AddWindow( std::move( window ) );
	return result;
}

void DoWindowsMenu();
void DoWindows( IFrameContext& frame_context );
void CloseAllWindows();

}
