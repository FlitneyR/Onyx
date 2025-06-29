#include "ChronoCommon/SystemContexts.h"
#include "Common/Assets.h"
#include "Common/Graphics/Camera.h"
#include "Common/Graphics/RenderTarget.h"
#include "Common/LowLevel/LowLevelInterface.h"
#include "Common/Scripting/Script.h"
#include "Common/Clock.h"

#include "Common/ECS/SystemContexts.h"
#include "Common/ECS/Systems/Rendering2D.h"
#include "Common/ECS/Systems/Transform.h"
#include "Common/ECS/SystemSet.h"
#include "Common/ECS/World.h"

#include "ChronoCommon/Systems/Camera.h"
#include "ChronoCommon/Systems/Physics.h"
#include "ChronoCommon/Systems/Player.h"

#define EDITOR_WINDOWS( f )
#include "Common/Editor/WindowImpl.h"

int main( int argc, const char** argv )
{
	INFO( "Loading Onyx core asset pack" );
	std::ifstream core_asset_pack_file( "../Common/onyx.pack", std::ios::binary | std::ios::beg );
	if ( !WEAK_ASSERT( core_asset_pack_file.is_open(), "onyx.pack file missing" ) )
		return -1;

	BjSON::Decoder core_decoder( core_asset_pack_file );
	onyx::AssetManager core_asset_manager( core_decoder.GetRootObject() );

	INFO( "Initialising Chrono" );

	onyx::LowLevel::Config config;
	onyx::LowLevel::Init( config, &core_asset_manager );

	onyx::LowLevelInput& input = onyx::LowLevel::GetInput();
	onyx::IWindowManager& window_manager = onyx::LowLevel::GetWindowManager();
	onyx::IGraphicsContext& graphics_context = onyx::LowLevel::GetGraphicsContext();

	{
		INFO( "Creating World" );
		onyx::ecs::World world;
		onyx::ecs::CommandBuffer cmd( world );

		onyx::ecs::QuerySet game_query_set( world );
		onyx::ecs::SystemSet< onyx::ecs::CommandBuffer, onyx::Tick, onyx::Camera2D > pre_tick_set( game_query_set );
		onyx::ecs::SystemSet< onyx::ecs::CommandBuffer, onyx::Tick, onyx::Camera2D > tick_set( game_query_set );
		onyx::ecs::SystemSet< onyx::ecs::CommandBuffer, onyx::Tick, onyx::Camera2D > post_tick_set( game_query_set );

		onyx::ecs::QuerySet render_query_set( world );
		onyx::ecs::SystemSet< onyx::SpriteRenderData, onyx::Camera2D > prerender_set( render_query_set );

		pre_tick_set.AddSubset( chrono::UpdatePhysicsBodies, chrono::UpdateCollisions );
		pre_tick_set.AddSystem( chrono::UpdateCamera );
		tick_set.AddSystem( chrono::UpdatePlayers );
		tick_set.AddSystem( onyx::UpdateAnimatedSprites );
		post_tick_set.AddSystem( onyx::UpdateTransform2DLocales );
		post_tick_set.AddSystem( onyx::UpdateParallaxBackgroundLayers );
		prerender_set.AddSystem( onyx::CollectSprites );

		INFO( "Loading Chrono asset pack" );
		std::ifstream chrono_asset_pack_file( "../ChronoCommon/Assets/chrono.pack", std::ios::binary | std::ios::beg );
		if ( !WEAK_ASSERT( chrono_asset_pack_file.is_open(), "chrono.pack file missing" ) )
			return -1;

		BjSON::Decoder chrono_decoder( chrono_asset_pack_file );
		onyx::AssetManager chrono_asset_manager( chrono_decoder.GetRootObject() );

		INFO( "Starting game" );
		{
			std::shared_ptr< onyx::Script > start_game_script = chrono_asset_manager.Load< onyx::Script >( "/start_game" );
			if ( !WEAK_ASSERT( start_game_script && start_game_script->GetLoadingState() == onyx::IAsset::LoadingState::Loaded, "start_game script missing" ) )
				return -1;

			onyx::ScriptContext start_game_context;
			start_game_context.AddInput( "Cmd", &cmd );
			start_game_context.AddInput( "AssetManager", &chrono_asset_manager );

			onyx::ScriptRunner start_game_script_runner( start_game_script, start_game_context );
			if ( !WEAK_ASSERT( start_game_script_runner.Run(), "start_game script failed" ) )
				return -1;

			cmd.Execute();
		}

		INFO( "Opening window" );
		onyx::IWindowManager::CreateWindowArgs create_window_args;
		// create_window_args.size = { 1920, 1080 };
		create_window_args.title = "Chrono";
		create_window_args.resizable = true;
		create_window_args.fullscreen = true;

		if ( std::shared_ptr< onyx::IWindow > game_window = WEAK_ASSERT( window_manager.CreateWindow( create_window_args ), "Failed to create game window" ) )
		{
			std::shared_ptr< onyx::IRenderTarget > render_target;
			std::shared_ptr< onyx::ISpriteRenderer > sprite_renderer = STRONG_ASSERT( graphics_context.CreateSpriteRenderer(), "Failed to create sprite renderer" );

			onyx::Clock clock;

			onyx::Tick tick_data;
			tick_data.deltaTime = 0.f;
			tick_data.time = 0.f;
			tick_data.frame = 0;

			onyx::Camera2D camera;

			while ( !window_manager.WantsToQuit() )
			{
				window_manager.ProcessEvents();

				input.UpdateButtonStates();

				clock.Tick();
				tick_data.frame += 1;
				tick_data.time = clock.GetTime();
				tick_data.deltaTime = clock.GetDeltaTime();

				game_query_set.Update();
				pre_tick_set.Run( cmd, tick_data, camera );
				tick_set.Run( cmd, tick_data, camera );
				post_tick_set.Run( cmd, tick_data, camera );

				if ( onyx::IFrameContext* frame_context = graphics_context.BeginFrame( *game_window ) )
				{
					const glm::uvec2 target_resolution = frame_context->GetSize();
					if ( !render_target || render_target->GetSize() != target_resolution )
						render_target = STRONG_ASSERT( graphics_context.CreateRenderTarget( target_resolution ), "Failed to create render target" );

					onyx::SpriteRenderData sprite_render_data;

					camera.aspectRatio = glm::normalize( glm::vec2( render_target->GetSize() ) );
					sprite_render_data.cameraMatrix = camera.GetMatrix();

					render_query_set.Update();
					prerender_set.Run( sprite_render_data, camera );

					render_target->Clear( *frame_context, {} );
					sprite_renderer->Render( *frame_context, render_target, sprite_render_data );
					frame_context->BlitRenderTarget( render_target, {}, frame_context->GetSize() );

					graphics_context.EndFrame( *frame_context );
				}

				cmd.Execute();
			}
		}
	}

	INFO( "Shutting Down Chrono" );
	onyx::LowLevel::CleanUp();

	return 0;
}
