#include "Onyx/Assets.h"
#include "Onyx/Clock.h"
#include "Onyx/Random.h"
#include "Onyx/Multithreading.h"
#include "Onyx/Graphics/Camera.h"
#include "Onyx/Graphics/RenderTarget.h"
#include "Onyx/LowLevel/LowLevelInterface.h"

#include "Onyx/ECS/Modules/Core.h"
#include "Onyx/ECS/Modules/Graphics2D.h"
#include "Onyx/ECS/Scene.h"
#include "Onyx/ECS/SystemContexts.h"
#include "Onyx/ECS/SystemSet.h"
#include "Onyx/ECS/World.h"

#include "Asteroids/Common/Modules/Core.h"
#include "Asteroids/Common/Modules/Physics.h"
#include "Asteroids/Common/Modules/Player.h"

#include "Asteroids/Common/RegisterReflectors.h"

#define EDITOR_WINDOWS( f )
#include "Onyx/Editor/WindowImpl.h"

#include "tracy/Tracy.hpp"

int main( int argc, const char** argv )
{
	INFO( "Registering Component Reflectors" );
	RegisterReflectors();

	INFO( "Loading Onyx core asset pack" );
	std::ifstream core_asset_pack_file( "../../Onyx/onyx.pack", std::ios::binary | std::ios::beg );
	if ( !WEAK_ASSERT( core_asset_pack_file.is_open(), "onyx.pack file missing" ) )
		return -1;

	BjSON::Decoder core_decoder( core_asset_pack_file );
	onyx::AssetManager core_asset_manager( core_decoder.GetRootObject() );

	INFO( "Initialising Asteroids" );

	onyx::LowLevel::Config config;
	onyx::LowLevel::Init( config, &core_asset_manager );

	onyx::LowLevelInput& input = onyx::LowLevel::GetInput();
	onyx::IWindowManager& window_manager = onyx::LowLevel::GetWindowManager();
	onyx::IGraphicsContext& graphics_context = onyx::LowLevel::GetGraphicsContext();

	{
		INFO( "Loading Asteroids asset pack" );
		std::ifstream asteroids_asset_pack_file( "../Common/Assets/asteroids.pack", std::ios::binary | std::ios::beg );
		if ( !WEAK_ASSERT( asteroids_asset_pack_file.is_open(), "asteroids.pack file missing" ) )
			return -1;

		BjSON::Decoder asteroids_decoder( asteroids_asset_pack_file );
		onyx::AssetManager asteroids_asset_manager( asteroids_decoder.GetRootObject() );

		INFO( "Creating World" );
		onyx::ecs::World world;
		onyx::ecs::CommandBuffer cmd( world );

		onyx::ecs::QuerySet tick_query_set( world );
		onyx::ecs::SystemSet<
			const onyx::ecs::World,
			onyx::ecs::CommandBuffer,
			onyx::AssetManager,
			const onyx::Tick,
			onyx::Camera2D,
			onyx::RNG
		> tick_set( tick_query_set );

		onyx::ecs::QuerySet render_query_set( world );
		onyx::ecs::SystemSet< onyx::SpriteRenderData, onyx::Camera2D > prerender_set( render_query_set );

		{
			INFO( "Registering systems and dependencies" );
			ZoneScopedN( "Register systems and dependencies" );

			tick_set.AddSystem( asteroids::Physics::UpdatePhysicsBodies );
			tick_set.AddSystem( asteroids::Physics::UpdateCollisions );
			tick_set.AddDependency( asteroids::Physics::UpdatePhysicsBodies, asteroids::Physics::UpdateCollisions );

			tick_set.AddSystem( asteroids::Core::UpdateCamera );
			tick_set.AddSystem( asteroids::Core::UpdateOffScreenSpawners );
			tick_set.AddDependency( asteroids::Core::UpdateCamera, asteroids::Core::UpdateOffScreenSpawners );

			tick_set.AddSystem( asteroids::Physics::UpdateDamageOnCollision );
			tick_set.AddSystem( asteroids::Player::UpdatePlayers );
			tick_set.AddSystem( onyx::Graphics2D::UpdateAnimatedSprites );

			tick_set.AddSystem( onyx::Core::UpdateTransform2DLocales );
			tick_set.AddDependency( asteroids::Physics::UpdatePhysicsBodies, onyx::Core::UpdateTransform2DLocales );

			tick_set.AddSystem( onyx::Graphics2D::UpdateParallaxBackgroundLayers );
			tick_set.AddDependency( onyx::Graphics2D::UpdateAnimatedSprites, onyx::Graphics2D::UpdateParallaxBackgroundLayers );

			tick_set.AddSystem( asteroids::Core::UpdateLifetimes );

			prerender_set.AddSystem( onyx::Graphics2D::CollectSprites );
		}

		INFO( "Loading entry point scene" );
		{
			ZoneScopedN( "Load entry point scene" );

			auto entry_point = asteroids_asset_manager.Load< onyx::ecs::Scene >( "/entry_point" );
			if ( !WEAK_ASSERT( entry_point, "Missing entry point scene" ) )
				return -1;

			cmd.CopySceneToWorld( entry_point );
			cmd.Execute();
		}

		INFO( "Opening window" );
		onyx::IWindowManager::CreateWindowArgs create_window_args;
		create_window_args.size = { 1920, 1080 };
		create_window_args.title = "Asteroids";
		create_window_args.resizable = true;
		// create_window_args.fullscreen = true;

		if ( std::shared_ptr< onyx::IWindow > game_window = WEAK_ASSERT( window_manager.CreateWindow( create_window_args ), "Failed to create game window" ) )
		{
			std::shared_ptr< onyx::IRenderTarget > render_target = nullptr;
			std::shared_ptr< onyx::ISpriteRenderer > sprite_renderer = STRONG_ASSERT( graphics_context.CreateSpriteRenderer(), "Failed to create sprite renderer" );

			onyx::Clock clock;

			onyx::Tick tick_data;
			tick_data.deltaTime = 0.f;
			tick_data.time = 0.f;
			tick_data.frame = 0;

			onyx::Camera2D camera;

			onyx::RNG rng( clock.GetUnixTime() );

			while ( !window_manager.WantsToQuit() )
			{
				window_manager.ProcessEvents();

				input.UpdateButtonStates();

				clock.Tick();
				tick_data.frame += 1;
				tick_data.time = clock.GetTime();
				tick_data.deltaTime = clock.GetDeltaTime();

				tick_query_set.Update();
				tick_set.Run( world, cmd, asteroids_asset_manager, tick_data, camera, rng );

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

					// wait for the rendering systems to finish
					onyx::LowLevel::GetWorkerPool().Wait();

					sprite_renderer->Render( *frame_context, render_target, sprite_render_data );
					frame_context->BlitRenderTarget( render_target, {}, frame_context->GetSize() );

					graphics_context.EndFrame( *frame_context );
				}

				world.CleanUpPages();
				cmd.Execute();
				
				FrameMark;
			}
		}
	}

	INFO( "Shutting Down Asteroids" );
	onyx::LowLevel::CleanUp();

	return 0;
}
