#include "Common/LowLevel/LowLevelInput.h"
#include "Common/Assets.h"
#include "Common/Graphics/Texture.h"
#include "ChronoEditor/WorldPreviewWindow.h"

// add your window here
// columns are: 
// 1) a window class
// 2) a name for this window in the windows menu
#define EDITOR_WINDOWS( f ) \
	f( ImGuiDemoWindow ) \
	f( onyx::LowLevelInput::DebugWindow ) \
	f( onyx::AssetManagerWindow ) \
	f( chrono::WorldPreviewWindow ) \

#include "Common/Editor/WindowImpl.h"
