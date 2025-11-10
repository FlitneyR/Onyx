#include "Onyx/Assets.h"
#include "Onyx/LowLevel/LowLevelInput.h"

// add your window here
#define EDITOR_WINDOWS( f ) \
	f( ImGuiDemoWindow ) \
	f( onyx::LowLevelInput::DebugWindow ) \
	f( onyx::AssetManagerWindow ) \

#include "Onyx/Editor/WindowImpl.h"
