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

struct WorldScriptPreviewWindow final : onyx::editor::IWindow
{
	WorldScriptPreviewWindow();

	std::shared_ptr< onyx::Script > m_script;

	onyx::ecs::World m_world;
	onyx::ecs::QuerySet m_tickQueries { m_world };
	onyx::ecs::QuerySet m_renderQueries { m_world };
	onyx::ecs::SystemSet< onyx::Tick, onyx::Camera2D > m_tickSet { m_tickQueries };
	onyx::ecs::SystemSet< onyx::Tick, onyx::Camera2D > m_postTickSet { m_tickQueries };
	onyx::ecs::SystemSet< onyx::SpriteRenderData, onyx::Camera2D > m_renderSet { m_renderQueries };

	std::shared_ptr< onyx::IRenderTarget > m_renderTarget;
	std::shared_ptr< onyx::ISpriteRenderer > m_spriteRenderer;

	onyx::Clock m_clock;

	float m_zoom = 1.f;
	glm::vec2 m_pan = {};

	void Refresh();

	inline static const char* const s_name = "World Preview Window";
	const char* GetName() const { return s_name; }
	void Run( onyx::IFrameContext& frame_context ) override;

	std::string GetWindowTitle() const override
	{ return std::format( "{}: {}###{}", s_name, !m_script ? "" : m_script->m_path, (u64)this ); }
};

}
