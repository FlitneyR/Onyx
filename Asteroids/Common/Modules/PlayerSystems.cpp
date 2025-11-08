#include "Player.h"

#include "Asteroids/Common/Modules/Core.h"

#include "Onyx/LowLevel/LowLevelInterface.h"

#include "tracy/Tracy.hpp"

namespace asteroids::Player
{

void UpdatePlayers( UpdatePlayers_Context ctx, const PlayerQuery& players, const PlayerEngineQuery& engines )
{
	ZoneScoped;

	auto [tick, asset_manager, cmd] = ctx.Break();

	const onyx::LowLevelInput& input = onyx::LowLevel::GetInput();

	f32 turn_input = 0.f;
	f32 boost_input = 0.f;
	bool fire_input = false;

	turn_input += input.GetAxisState( onyx::InputAxis::GamepadAxis_LeftStickX ).x;
	turn_input += input.GetAxisState( onyx::InputAxis::Keyboard_RightArrow ).x;
	turn_input -= input.GetAxisState( onyx::InputAxis::Keyboard_LeftArrow ).x;
	turn_input += input.GetAxisState( onyx::InputAxis::Keyboard_D ).x;
	turn_input -= input.GetAxisState( onyx::InputAxis::Keyboard_A ).x;

	boost_input += input.GetAxisState( onyx::InputAxis::GamepadAxis_RightTrigger ).x;
	boost_input += input.GetAxisState( onyx::InputAxis::Keyboard_UpArrow ).x;
	boost_input += input.GetAxisState( onyx::InputAxis::Keyboard_W ).x;

	fire_input |= input.GetButtonState( onyx::InputAxis::GamepadButton_FaceLeft ) == onyx::ButtonState::Pressed;
	fire_input |= input.GetButtonState( onyx::InputAxis::Keyboard_Space ) == onyx::ButtonState::Pressed;

	turn_input = std::clamp< f32 >( turn_input, -1.f, 1.f );
	boost_input = std::clamp< f32 >( boost_input, 0.f, 1.f );

	for ( auto& player : players )
	{
		auto [id, pc, transform, body] = player.Break();

		body.angularVelocity += turn_input * pc.turnSpeed * tick.deltaTime;
		body.linearVelocity += transform.GetRelative( { 0.f, 1.f, 0.f } ) * boost_input * pc.boostSpeed * tick.deltaTime;

		auto required_animation = boost_input < 0.25f ? pc.idleEngineAnimation : pc.boostEngineAnimation;

		if ( auto* engine = engines.Get( pc.engineEffectSprite ) )
		{
			auto [id, animator] = engine->Break();

			if ( required_animation && animator.animation != required_animation )
			{
				animator.currentFrame = 0.f;
				animator.animation = required_animation;
				animator.playRate = required_animation->m_rate;
			}
		}

		if ( fire_input && pc.bulletPrefab )
		{
			using onyx::ecs::World;
			using onyx::ecs::Scene;
			using onyx::ecs::IDMap;

			if ( pc.bulletPrefab->GetLoadingState() != onyx::LoadingState::Loaded )
				pc.bulletPrefab->Load( onyx::IAsset::LoadType::Stream );

			cmd.CopySceneToWorld( pc.bulletPrefab,
				[
					base_position = transform.GetLocalPosition(),
					base_rotation = transform.GetLocalRotation(),
					base_velocity = body.linearVelocity
				]
				( World& world, const IDMap& id_map )
				{
					using onyx::Core::Transform2D;
					using onyx::Core::AttachedTo;
					using Physics::PhysicsBody;
					using Core::Team;

					for ( auto& [_, entity] : id_map )
					{
						if ( Team* const team = world.GetComponent< Team >( entity ) )
							team->team = Team::Player;
						else
							world.AddComponent( entity, Team( Team::Player ) );

						if ( Transform2D* const transform = world.GetComponent< Transform2D >( entity ) )
						{
							// ignore child entities
							if ( world.GetComponent< AttachedTo >( entity ) )
								continue;

							transform->SetLocalPosition( transform->GetLocalPosition() + base_position );
							transform->SetLocalRotation( transform->GetLocalRotation() + base_rotation );

							if ( const Projectile* const projectile = world.GetComponent< Projectile >( entity ) )
							{
								if ( PhysicsBody* const pb = world.GetComponent< PhysicsBody >( entity ) )
								{
									glm::vec2 velocity = transform->GetRelative( { 0.f, -projectile->initialSpeed, 0.f } );
									velocity += base_velocity * glm::dot( normalize( velocity ), normalize( base_velocity ) );

									pb->linearVelocity = velocity;
								}
							}
						}
					}
				}
			);
		}
	}
}

}
