#pragma once

#include "Common/Graphics/GraphicsContext.h"
#include "Common/LowLevel/LowLevelInput.h"
#include "Common/LowLevel/WindowManager.h"

namespace onyx
{

namespace LowLevel
{

struct Config
{
	enum struct WindowManager
	{
		SDL = 0,
	} windowManager = WindowManager::SDL;

	enum struct GraphicsAPI
	{
		Vulkan = 0,
	} graphicsAPI = GraphicsAPI::Vulkan;

	bool enableImGui = false;
};

void Init( const Config& config );
void CleanUp();

const Config& GetConfig();

IGraphicsContext& GetGraphicsContext();
LowLevelInput& GetInput();
IWindowManager& GetWindowManager();

}

}
