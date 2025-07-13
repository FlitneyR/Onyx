#include "ScenePreviewer.h"

#include "Common/ECS/Modules/Core.h"
#include "Common/ECS/Modules/Graphics2D.h"

#include "Common/Graphics/RenderTarget.h"

#include "Common/LowLevel/LowLevelInterface.h"

namespace chrono
{

ScenePreviewer::Factory ScenePreviewer::s_factory;

ScenePreviewer::ScenePreviewer( onyx::ecs::World& world )
	: m_tickQuerySet( world )
	, m_renderQuerySet( world )
	, m_tickSystemSet( m_tickQuerySet )
	, m_renderSystemSet( m_renderQuerySet )
{
	m_tickSystemSet.AddSubset(
		onyx::Graphics2D::UpdateAnimatedSprites,
		onyx::Core::UpdateTransform2DLocales,
		onyx::Graphics2D::UpdateParallaxBackgroundLayers
	);

	m_renderSystemSet.AddSystem( onyx::Graphics2D::CollectSprites );

	m_spriteRenderer = onyx::LowLevel::GetGraphicsContext().CreateSpriteRenderer();

	m_camera.fov = 2.f;
}

void ScenePreviewer::Tick( onyx::IFrameContext& frame_context, std::shared_ptr< onyx::IRenderTarget >& render_target )
{
	if ( ImGui::BeginMenuBar() )
	{
		if ( ImGui::BeginMenu( "Camera" ) )
		{
			ImGui::DragFloat( "FOV", &m_camera.fov, 0.05f, 0.f, 1'000'000.f );
			ImGui::DragFloat2( "Position", &m_camera.position.x, 0.05f );

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	if ( ImGui::IsWindowFocused() )
	{
		onyx::LowLevelInput& input = onyx::LowLevel::GetInput();

		const ImVec2 window_size = ImGui::GetWindowSize();
		const f32 scale = 2.f * m_camera.fov / glm::length( glm::vec2( window_size.x, window_size.y ) );

		if ( input.IsButtonDown( onyx::InputAxis::MouseButton_Middle ) )
			m_camera.position -= input.GetAxisState( onyx::InputAxis::MouseAxis_XY ) * glm::vec2( scale, -scale );
	}

	m_clock.Tick();
	m_tick.frame += 1;
	m_tick.time = m_clock.GetTime();
	m_tick.deltaTime = m_clock.GetDeltaTime();

	m_tickQuerySet.Update();
	m_tickSystemSet.Run( m_tick, m_camera );

	onyx::SpriteRenderData sprite_render_data;
	m_camera.aspectRatio = glm::normalize( glm::vec2( render_target->GetSize() ) );
	sprite_render_data.cameraMatrix = m_camera.GetMatrix();

	m_renderQuerySet.Update();
	m_renderSystemSet.Run( sprite_render_data );

	render_target->Clear( frame_context, {} );
	m_spriteRenderer->Render( frame_context, render_target, sprite_render_data );
}

}
