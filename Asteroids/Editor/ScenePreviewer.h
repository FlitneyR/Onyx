#pragma once

#include "Onyx/Clock.h"

#include "Onyx/ECS/CommandBuffer.h"
#include "Onyx/ECS/Query.h"
#include "Onyx/ECS/SystemContexts.h"
#include "Onyx/ECS/SystemSet.h"

#include "Onyx/Graphics/Camera.h"
#include "Onyx/Graphics/SpriteRenderer.h"

#include "Onyx/ECS/Scene.h"

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
