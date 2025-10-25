#include "LowLevelInterface.h"

#include "Impl/SDLWindowManager.h"
#include "Impl/SDLInput.h"
#include "Impl/VulkanGraphicsContext.h"

#include "imgui.h"

namespace onyx
{

namespace LowLevel
{

static bool s_isReady = false;

IGraphicsContext* s_graphicsContext = nullptr;
LowLevelInput* s_lowLevelInput = nullptr;
IWindowManager* s_windowManager = nullptr;

AssetManager* s_coreAssetManager = nullptr;

Config s_config = {};

void Init( const Config& config, AssetManager* core_asset_manager )
{
	ASSERT( !s_isReady );

	s_config = config;
	s_coreAssetManager = core_asset_manager; 

	if ( config.enableImGui )
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		// io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		style.TabRounding = 4.f;
		style.FrameRounding = 4.f;
		style.GrabRounding = 4.f;
		style.WindowRounding = 4.f;
		style.PopupRounding = 4.f;
		style.Colors[ ImGuiCol_DockingEmptyBg ] = ImVec4( 0.05f, 0.05f, 0.05f, 1.0f );
		// style.Colors[]
	}

	switch ( config.windowManager )
	{
	case Config::WindowManager::SDL:
		STRONG_ASSERT( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER ) == 0, "Failed to initialise SDL: {}", SDL_GetError() );
		s_windowManager = new SDLWindowManager();
		s_lowLevelInput = new SDLInput();
		break;
	}

	switch ( config.graphicsAPI )
	{
	case Config::GraphicsAPI::Vulkan:
		s_graphicsContext = new VulkanGraphicsContext();
		break;
	}

	s_isReady = true;
}

void CleanUp()
{
	ASSERT( s_isReady );

	delete s_windowManager;
	delete s_graphicsContext;
	delete s_lowLevelInput;

	s_isReady = false;
}

const Config& GetConfig()
{
	ASSERT( s_isReady );
	return s_config;
}

IGraphicsContext& GetGraphicsContext()
{
	ASSERT( s_isReady );
	return *ASSERT( s_graphicsContext );
}

LowLevelInput& GetInput()
{
	ASSERT( s_isReady );
	return *ASSERT( s_lowLevelInput );
}

IWindowManager& GetWindowManager()
{
	ASSERT( s_isReady );
	return *ASSERT( s_windowManager );
}

AssetManager* GetCoreAssetManager()
{
	return s_coreAssetManager;
}

}

}
