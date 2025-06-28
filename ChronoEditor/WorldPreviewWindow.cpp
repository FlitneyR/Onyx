#include "WorldPreviewWindow.h"

#include "Common/LowLevel/LowLevelInterface.h"

#include "Common/ECS/Systems/Transform.h"
#include "Common/ECS/Systems/Rendering2D.h"

#include "Common/Graphics/RenderTarget.h"

#include "imgui_stdlib.h"

namespace chrono
{

WorldPreviewWindow::WorldPreviewWindow()
{
	m_tickSet.AddSystem( onyx::UpdateTransform2DLocales );
	m_tickSet.AddSystem( onyx::UpdateAnimatedSprites );
	m_postTickSet.AddSystem( onyx::UpdateParallaxBackgroundLayers );
	m_renderSet.AddSystem( onyx::CollectSprites );

	m_spriteRenderer = onyx::LowLevel::GetGraphicsContext().CreateSpriteRenderer();
}

void WorldPreviewWindow::Run( onyx::IFrameContext& frame_context )
{
	if ( ImGui::Begin( GetWindowTitle().c_str(), &m_open, ImGuiWindowFlags_MenuBar ) )
	{
		if ( ImGui::BeginMenuBar() )
		{
			if ( ImGui::BeginMenu( "File" ) )
			{
				ImGui::InputText( "Script Path", &m_scriptPath );

				if ( ImGui::MenuItem( "Load" ) )
				{
					const std::string new_file = onyx::LowLevel::GetWindowManager().DoOpenFileDialog();
					if ( !new_file.empty() )
						Load( new_file, m_scriptPath );
				}

				if ( ImGui::MenuItem( "Reload" ) )
					Load( m_assetPackPath, m_scriptPath );

				ImGui::EndMenu();
			}

			if ( ImGui::BeginMenu( "View" ) )
			{
				ImGui::DragFloat( "Zoom", &m_zoom, 0.0025, 0.001, 1.0, "%.6f" );
				ImGui::DragFloat2( "Pan", &m_pan.x );

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		m_clock.Tick();

		ImVec2 window_size = ImGui::GetWindowSize();
		window_size.x -= 20;
		window_size.y -= 50;

		if ( window_size.x > 0 && window_size.y > 0 )
		{
			if ( !m_renderTarget || window_size.x != m_renderTarget->GetSize().x || window_size.y != m_renderTarget->GetSize().y )
				m_renderTarget = STRONG_ASSERT( onyx::LowLevel::GetGraphicsContext().CreateRenderTarget( { window_size.x, window_size.y } ) );

			onyx::Tick tick_data;
			tick_data.deltaTime = m_clock.GetDeltaTime();
			tick_data.time = 0.f;

			onyx::Camera2D camera;
			camera.position = m_pan;
			camera.aspectRatio = glm::normalize( glm::vec2( m_renderTarget->GetSize() ) );
			camera.fov = 1.f / m_zoom;

			onyx::SpriteRenderData render_data;
			render_data.cameraMatrix = camera.GetMatrix();

			m_tickQueries.Update();
			m_tickSet.Run( tick_data, camera );
			m_postTickSet.Run( tick_data, camera );

			m_renderQueries.Update();
			m_renderSet.Run( render_data, camera );

			m_renderTarget->Clear( frame_context, {} );
			m_spriteRenderer->Render( frame_context, m_renderTarget, render_data );
			m_renderTarget->PrepareForSampling( frame_context );

			ImGui::Image( m_renderTarget->GetImTextureID(), window_size );
			frame_context.RegisterUsedResource( m_renderTarget );
		}
	}

	ImGui::End();
}

void WorldPreviewWindow::Load( std::string asset_pack_path, std::string script_path )
{
	m_assetPackPath = asset_pack_path;
	m_scriptPath = script_path;

	m_assetPackFile = std::ifstream( m_assetPackPath, std::ios::binary );
	m_assetPackDecoder = std::make_unique< BjSON::Decoder >( m_assetPackFile );
	m_assetPackLoader = std::make_unique< onyx::AssetLoader >( m_assetPackDecoder->GetRootObject() );

	m_clock = onyx::Clock();
	m_world.ResetEntities();

	if ( std::shared_ptr< onyx::Script > script = LOG_ASSERT( m_assetPackLoader->Load< onyx::Script >( script_path ), "Couldn't load {} from {}", script_path, asset_pack_path ) )
	{
		onyx::ScriptContext script_ctx;

		onyx::ecs::CommandBuffer command_buffer( m_world );
		script_ctx.AddInput( "Cmd", &command_buffer );
		script_ctx.AddInput( "AssetLoader", m_assetPackLoader.get() );

		onyx::ScriptRunner script_runner( script, script_ctx );
		if ( WEAK_ASSERT( script_runner.Run() ) )
			command_buffer.Execute();
	}
}

}
