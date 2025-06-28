#pragma once

#include "Common/Assets.h"
#include "Common/ECS/SystemContexts.h"
#include "Common/ECS/SystemSet.h"
#include "Common/ECS/World.h"
#include "Common/Editor/Window.h"
#include "Common/Graphics/Camera.h"
#include "Common/Graphics/SpriteRenderer.h"
#include "Common/Scripting/Script.h"
#include "Common/Clock.h"

namespace chrono
{

struct WorldPreviewWindow final : onyx::editor::IWindow
{
	onyx::ecs::World m_world;
	onyx::ecs::QuerySet m_tickQueries { m_world };
	onyx::ecs::QuerySet m_renderQueries { m_world };
	onyx::ecs::SystemSet< onyx::Tick, onyx::Camera2D > m_tickSet { m_tickQueries };
	onyx::ecs::SystemSet< onyx::Tick, onyx::Camera2D > m_postTickSet { m_tickQueries };
	onyx::ecs::SystemSet< onyx::SpriteRenderData, onyx::Camera2D > m_renderSet { m_renderQueries };

	WorldPreviewWindow();

	std::string m_assetPackPath;
	std::string m_scriptPath = "/start_game";

	std::ifstream m_assetPackFile;
	std::unique_ptr< BjSON::Decoder > m_assetPackDecoder;
	std::unique_ptr< onyx::AssetLoader > m_assetPackLoader;

	std::shared_ptr< onyx::IRenderTarget > m_renderTarget;
	std::shared_ptr< onyx::ISpriteRenderer > m_spriteRenderer;

	onyx::Clock m_clock;

	float m_zoom = 1.f;
	glm::vec2 m_pan = {};

	void Load( std::string asset_pack_path, std::string script_path );

	inline static const char* const s_name = "World Preview Window";
	const char* GetName() const { return s_name; }
	void Run( onyx::IFrameContext& frame_context ) override;

	std::string GetWindowTitle() const override
	{ return std::format( "{}: {} {}###{}", s_name, m_scriptPath, m_assetPackPath, (u64)this ); }
};

}
