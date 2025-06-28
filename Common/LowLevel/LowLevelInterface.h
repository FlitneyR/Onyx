#pragma once

#include "Common/Graphics/GraphicsContext.h"
#include "Common/LowLevel/LowLevelInput.h"
#include "Common/LowLevel/WindowManager.h"

#include "Common/Assets.h"

namespace onyx
{

namespace LowLevel
{

struct Config
{
	enum struct WindowManager
	{
		SDL = 0,
		Count
	} windowManager = WindowManager::SDL;

	enum struct GraphicsAPI
	{
		Vulkan = 0,
		Count
	} graphicsAPI = GraphicsAPI::Vulkan;

	inline static const char* const s_GraphicsAPINames[] {
		"Vulkan",
	};

	bool enableImGui = false;
};

void Init( const Config& config, AssetLoader* core_asset_loader = nullptr );
void CleanUp();

const Config& GetConfig();

IGraphicsContext& GetGraphicsContext();
LowLevelInput& GetInput();
IWindowManager& GetWindowManager();
AssetLoader* GetCoreAssetLoader();

}

}
