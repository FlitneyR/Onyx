#include "Onyx/Assets.h"
#include "Onyx/LowLevel/LowLevelInput.h"

// add your window here
// columns are: 
// 1) a window class
// 2) a name for this window in the windows menu
#define EDITOR_WINDOWS( f ) \
	f( ImGuiDemoWindow ) \
	f( onyx::LowLevelInput::DebugWindow ) \
	f( onyx::AssetManagerWindow ) \

#include "Onyx/Editor/WindowImpl.h"
