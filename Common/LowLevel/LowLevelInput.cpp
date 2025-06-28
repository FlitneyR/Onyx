#include "LowLevelInput.h"
#include "LowLevelInterface.h"
#include "imgui.h"

#include "Common/Editor/Window.h"

namespace onyx
{

f32 LowLevelInput::s_buttonSensitivity = 0.5f;
f32 LowLevelInput::s_deadZone = 0.125f;

LowLevelInput::LowLevelInput()
{
	std::memset( &m_axisStates, 0, sizeof( m_axisStates ) );
	std::memset( &m_buttonStates, 0, sizeof( m_buttonStates ) );
}

void LowLevelInput::UpdateButtonStates()
{
	for ( u32 axis = 0; axis < (u32)InputAxis::Count; ++axis )
	{
		m_buttonStates[ axis ] = ButtonState(
			// turn IsDown into WasDown
			( (u8)ButtonState::WasDown * ( (u8)m_buttonStates[ axis ] & (u8)ButtonState::IsDown ) ) |
			// set IsDown if the axis is currently more than half activated
			( (u8)ButtonState::IsDown * (u8)( dot( m_axisStates[ axis ], m_axisStates[ axis ] ) > s_buttonSensitivity * s_buttonSensitivity ) )
		);
	}
}

std::string LowLevelInput::DebugWindow::GetWindowTitle() const
{
	return std::format( "Input Debug###{}", (u64)this );
}

void LowLevelInput::DebugWindow::Run( IFrameContext& frame_context )
{
	if ( ImGui::Begin( GetWindowTitle().c_str(), &m_open ) )
	{
		ImGui::SetWindowSize( { 1000, 600 }, ImGuiCond_Once );

		ImGui::SliderFloat( "Button Sensitivity", &s_buttonSensitivity, 0.0f, 1.0f, "%.1f" );

		if ( ImGui::Button( "Clear" ) )
			for ( bool& input : m_inputEnabled )
				input = false;

		ImGui::SameLine();
		if ( ImGui::BeginCombo( "##Input Toggle", "Toggle Input" ) )
		{
			for ( onyx::InputAxis axis = onyx::InputAxis::None; axis < onyx::InputAxis::Count; ++(u32&)axis )
				ImGui::Selectable( onyx::GetInputAxisName( axis ), &m_inputEnabled[ (u32)axis ] );

			ImGui::EndCombo();
		}

		ImGui::SameLine();
		if ( ImGui::BeginCombo( "##Enable All Inputs For Device", "Device" ) )
		{
			for ( onyx::InputDevice device = onyx::InputDevice::None; device < onyx::InputDevice::Count; ++(u32&)device )
			{
				bool selected = false;
				ImGui::Selectable( onyx::GetInputDeviceName( device ), &selected );
				if ( selected )
					for ( onyx::InputAxis axis = onyx::InputAxis::None; axis < onyx::InputAxis::Count; ++(u32&)axis )
						if ( onyx::GetInputDeviceForAxis( axis ) == device )
							m_inputEnabled[ (u32)axis ] = true;
			}

			ImGui::EndCombo();
		}

		if ( ImGui::BeginChild( "Input List", {}, 0, ImGuiWindowFlags_AlwaysVerticalScrollbar ) )
		{
			if ( ImGui::BeginTable( "Input states", 4 ) )
			{
				for ( onyx::InputAxis axis = onyx::InputAxis::None; axis < onyx::InputAxis::Count; ++(u32&)axis )
				{
					if ( !m_inputEnabled[ (u32)axis ] )
						continue;

					const glm::vec2 state = onyx::LowLevel::GetInput().GetAxisState( axis );
					const char* button_state = "None";

					switch ( onyx::LowLevel::GetInput().GetButtonState( axis ) )
					{
					case onyx::ButtonState::NotHeld: button_state = "Not Down"; break;
					case onyx::ButtonState::Pressed: button_state = "Pressed"; break;
					case onyx::ButtonState::Released: button_state = "Released"; break;
					case onyx::ButtonState::Held: button_state = "Held"; break;
					}

					ImGui::PushID( (u32)axis );
					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					if ( ImGui::Button( "[x]" ) )
						m_inputEnabled[ (u32)axis ] = false;

					ImGui::SameLine();
					ImGui::Text( "%s", onyx::GetInputAxisName( axis ) );

					ImGui::TableNextColumn();
					ImGui::Text( "%.1f", state.x );

					ImGui::TableNextColumn();
					ImGui::Text( "%.1f", state.y );

					ImGui::TableNextColumn();
					ImGui::Text( "%s", button_state );

					ImGui::PopID();
				}

				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
	}

	ImGui::End();
}

void LowLevelInput::SetAxisState( InputAxis axis, f32 state )
{
	// check for a valid input axis
	if ( !WEAK_ASSERT( axis < InputAxis::Count, "{} is not an input axis", (u32)axis ) )
		return;

	if ( axis <= InputAxis::None )
		return;

	// check that this input axis can be set
	switch ( axis )
	{
	case InputAxis::MouseAxis_XY:
	case InputAxis::MouseAxis_Up:
	case InputAxis::MouseAxis_Down:
	case InputAxis::MouseAxis_Left:
	case InputAxis::MouseAxis_Right:
	case InputAxis::GamepadButton_DPadCombined:
	case InputAxis::GamepadAxis_LeftStickXY:
	case InputAxis::GamepadAxis_LeftStickUp:
	case InputAxis::GamepadAxis_LeftStickDown:
	case InputAxis::GamepadAxis_LeftStickLeft:
	case InputAxis::GamepadAxis_LeftStickRight:
	case InputAxis::GamepadAxis_RightStickXY:
	case InputAxis::GamepadAxis_RightStickUp:
	case InputAxis::GamepadAxis_RightStickDown:
	case InputAxis::GamepadAxis_RightStickLeft:
	case InputAxis::GamepadAxis_RightStickRight:
		WEAK_ASSERT( false, "Input axis {}({}) is a derived input axis, and shouldn't be set directly", (u32)axis, GetInputAxisName( axis ) );
		return;
	default:
		break;
	}

	m_axisStates[ (u32)axis ].x = glm::sign( state ) * glm::clamp( glm::abs( state ) - s_deadZone, 0.f, 1.f );

	// update derived input axes
	switch ( axis )
	{
	case InputAxis::MouseAxis_X:
		m_axisStates[ (u32)InputAxis::MouseAxis_Right ].x = glm::max( 0.f, state );
		m_axisStates[ (u32)InputAxis::MouseAxis_Left ].x = glm::max( 0.f, -state );
		m_axisStates[ (u32)InputAxis::MouseAxis_XY ].x = state;
		break;
	case InputAxis::MouseAxis_Y:
		m_axisStates[ (u32)InputAxis::MouseAxis_Down ].x = glm::max( 0.f, state );
		m_axisStates[ (u32)InputAxis::MouseAxis_Up ].x = glm::max( 0.f, -state );
		m_axisStates[ (u32)InputAxis::MouseAxis_XY ].y = -state;
		break;
	case InputAxis::GamepadAxis_LeftStickX:
		m_axisStates[ (u32)InputAxis::GamepadAxis_LeftStickRight ].x = glm::max( 0.f, state );
		m_axisStates[ (u32)InputAxis::GamepadAxis_LeftStickLeft ].x = glm::max( 0.f, -state );
		m_axisStates[ (u32)InputAxis::GamepadAxis_LeftStickXY ].x = state;
		break;
	case InputAxis::GamepadAxis_LeftStickY:
		m_axisStates[ (u32)InputAxis::GamepadAxis_LeftStickUp ].x = glm::max( 0.f, state );
		m_axisStates[ (u32)InputAxis::GamepadAxis_LeftStickDown ].x = glm::max( 0.f, -state );
		m_axisStates[ (u32)InputAxis::GamepadAxis_LeftStickXY ].y = state;
		break;
	case InputAxis::GamepadAxis_RightStickX:
		m_axisStates[ (u32)InputAxis::GamepadAxis_RightStickRight ].x = glm::max( 0.f, state );
		m_axisStates[ (u32)InputAxis::GamepadAxis_RightStickLeft ].x = glm::max( 0.f, -state );
		m_axisStates[ (u32)InputAxis::GamepadAxis_RightStickXY ].x = state;
		break;
	case InputAxis::GamepadAxis_RightStickY:
		m_axisStates[ (u32)InputAxis::GamepadAxis_RightStickUp ].x = glm::max( 0.f, state );
		m_axisStates[ (u32)InputAxis::GamepadAxis_RightStickDown ].x = glm::max( 0.f, -state );
		m_axisStates[ (u32)InputAxis::GamepadAxis_RightStickXY ].y = state;
		break;
	case InputAxis::GamepadButton_DPadUp:
	case InputAxis::GamepadButton_DPadDown:
	case InputAxis::GamepadButton_DPadLeft:
	case InputAxis::GamepadButton_DPadRight:
		m_axisStates[ (u32)InputAxis::GamepadButton_DPadCombined ].x =
			m_axisStates[ (u32)InputAxis::GamepadButton_DPadRight ].x -
			m_axisStates[ (u32)InputAxis::GamepadButton_DPadLeft ].x;

		m_axisStates[ (u32)InputAxis::GamepadButton_DPadCombined ].y =
			m_axisStates[ (u32)InputAxis::GamepadButton_DPadUp ].x -
			m_axisStates[ (u32)InputAxis::GamepadButton_DPadDown ].x;

		break;
	default:
		break;
	}
}

}
