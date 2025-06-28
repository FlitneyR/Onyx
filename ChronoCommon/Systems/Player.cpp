#include "Player.h"

#include "Common/LowLevel/LowLevelInterface.h"

namespace chrono
{

void UpdatePlayers( onyx::ecs::Context< const onyx::Tick > ctx, const PlayerQuery& players, const PlayerEngineQuery& engines )
{
	auto [tick] = ctx.Break();

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
		body.linearVelocity += transform.GetRelative( glm::vec2( 0.f, 1.f ) ) * boost_input * pc.boostSpeed * tick.deltaTime;

		auto required_animation = boost_input < 0.25f ? pc.idleEngineAnimation : pc.boostEngineAnimation;

		if ( auto* engine = engines.Get( pc.engineSprite ) )
		{
			auto [id, animator] = engine->Break();

			if ( required_animation && animator.m_animation != required_animation )
			{
				animator.m_currentFrame = 0.f;
				animator.m_animation = required_animation;
				animator.m_playRate = required_animation->m_rate;
			}
		}
	}
}

}
