#include "Common/LowLevel/LowLevelInput.h"
#include "Common/Assets.h"
#include "Common/Graphics/Texture.h"

namespace chrono
{

}

// add your window here
// columns are: 
// 1) a window class
// 2) a name for this window in the windows menu
#define EDITOR_WINDOWS( f ) \
	f( ImGuiDemoWindow ) \
	f( onyx::LowLevelInput::DebugWindow ) \
	f( onyx::AssetManagerWindow ) \

#include "Common/Editor/WindowImpl.h"
