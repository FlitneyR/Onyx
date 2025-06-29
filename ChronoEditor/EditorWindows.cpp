#include "Common/LowLevel/LowLevelInput.h"
#include "Common/Assets.h"
#include "Common/Graphics/Texture.h"
#include "ChronoEditor/WorldScriptPreviewWindow.h"

namespace chrono
{

const static struct AssetManagerCallbacks : onyx::IAssetManagerCallbacks
{
	void PreviewSceneScript( std::shared_ptr< onyx::Script > script ) const override
	{
		auto window = onyx::editor::AddWindow< chrono::WorldScriptPreviewWindow >();
		window->m_script = script;
		window->Refresh();
	}
} s_assetManagerCallbacks;

}

// add your window here
// columns are: 
// 1) a window class
// 2) a name for this window in the windows menu
#define EDITOR_WINDOWS( f ) \
	f( ImGuiDemoWindow ) \
	f( onyx::LowLevelInput::DebugWindow ) \
	f( onyx::AssetManagerWindow, chrono::s_assetManagerCallbacks ) \

#include "Common/Editor/WindowImpl.h"
