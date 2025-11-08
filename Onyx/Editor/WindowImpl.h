#include "Window.h"

#include <vector>

namespace onyx::editor
{

std::vector< std::unique_ptr< IWindow > > s_windows;

struct ImGuiDemoWindow : IWindow
{
	inline static const char* const s_name = "ImGui Demo";
	void Run( IFrameContext& frame_context ) override { ZoneScoped; ImGui::ShowDemoWindow( &m_open ); }
	std::string GetWindowTitle() const override { return s_name; }
};

void AddWindow( std::unique_ptr< IWindow > window )
{
	s_windows.push_back( std::move( window ) );
}

void DoWindowsMenu()
{
	if ( ImGui::BeginMenu( "Windows" ) )
	{
		#define EDITOR_WINDOW_DO_MENU_ITEM( WindowType, ... )	\
			if ( ImGui::MenuItem( WindowType::s_name ) )		\
				AddWindow< WindowType >( __VA_ARGS__ );			\

		EDITOR_WINDOWS( EDITOR_WINDOW_DO_MENU_ITEM );

		if ( !s_windows.empty() )
			ImGui::Separator();

		for ( auto& window : s_windows )
		{
			const std::string window_title = window->GetWindowTitle();

			if ( ImGui::Selectable( std::format( "[x]##{}", window_title ).c_str(), false, 0, ImGui::CalcTextSize( "[x]" ) ) )
				window->m_open = false;

			ImGui::SameLine();
			if ( ImGui::Selectable( window_title.c_str() ) )
				ImGui::SetWindowFocus( window_title.c_str() );
		}

		ImGui::EndMenu();
	}
}

void DoWindows( IFrameContext& frame_context )
{
	for ( u32 window_index = 0; window_index < s_windows.size(); ++window_index )
		s_windows[ window_index ]->Run( frame_context );

	std::erase_if( s_windows, []( std::unique_ptr< IWindow >& window ) { return !window->m_open; } );
}

void CloseAllWindows()
{
	s_windows.clear();
}

}