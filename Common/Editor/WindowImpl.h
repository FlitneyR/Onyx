#include "Window.h"

#include <vector>

namespace onyx::editor
{

::std::vector< ::std::unique_ptr< IWindow > > s_windows;

struct ImGuiDemoWindow : IWindow
{
	inline static const char* const s_name = "ImGui Demo";
	virtual void Run() { ImGui::ShowDemoWindow( &m_open ); }
};

void DoWindowsMenu()
{
	if ( ImGui::BeginMenu( "Windows" ) )
	{
		#define EDITOR_WINDOW_DO_MENU_ITEM( WindowType ) \
			if ( ImGui::MenuItem( WindowType::s_name ) )					\
				s_windows.push_back( ::std::make_unique< WindowType >() );	\

		EDITOR_WINDOWS( EDITOR_WINDOW_DO_MENU_ITEM );

		ImGui::EndMenu();
	}
}

void DoWindows()
{
	::std::erase_if( s_windows, []( ::std::unique_ptr< IWindow >& window ) -> bool
	{
		window->Run();
		return !window->m_open;
	} );
}

}