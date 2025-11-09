#pragma once

#include "Onyx/Graphics/GraphicsContext.h"
#include "Onyx/LowLevel/LowLevelInput.h"
#include "Onyx/LowLevel/WindowManager.h"

#include "Onyx/Assets.h"

namespace onyx
{

// forward declaration from "Onyx/Multithreading.h"
struct WorkerPool;

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

	u32 numWorkers = 0;
};

void Init( const Config& config, AssetManager* core_asset_manager = nullptr );
void CleanUp();

const Config& GetConfig();

IGraphicsContext& GetGraphicsContext();
LowLevelInput& GetInput();
IWindowManager& GetWindowManager();
WorkerPool& GetWorkerPool();
AssetManager* GetCoreAssetManager();

}

}
