#pragma once

#include "Common/Clock.h"

#include "Common/ECS/CommandBuffer.h"
#include "Common/ECS/Query.h"
#include "Common/ECS/SystemContexts.h"
#include "Common/ECS/SystemSet.h"

#include "Common/Graphics/Camera.h"
#include "Common/Graphics/SpriteRenderer.h"

#include "Common/ECS/Scene.h"

namespace asteroids
{

struct ScenePreviewer : onyx::ecs::SceneEditor::IPreviewer
{
	static struct Factory : IFactory
	{
		std::unique_ptr< IPreviewer > MakePreviewer( onyx::ecs::World& world ) override
		{ return std::make_unique< ScenePreviewer >( world ); }
	} s_factory;

	onyx::ecs::QuerySet m_tickQuerySet;
	onyx::ecs::QuerySet m_renderQuerySet;
	onyx::ecs::SystemSet< onyx::Tick, onyx::Camera2D > m_tickSystemSet;
	onyx::ecs::SystemSet< onyx::SpriteRenderData > m_renderSystemSet;

	onyx::Clock m_clock;
	onyx::Tick m_tick;
	onyx::Camera2D m_camera;

	std::shared_ptr< onyx::ISpriteRenderer > m_spriteRenderer;

	ScenePreviewer( onyx::ecs::World& world );

	void Tick( onyx::IFrameContext& frame_context, std::shared_ptr< onyx::IRenderTarget >& render_target ) override;
};

}
