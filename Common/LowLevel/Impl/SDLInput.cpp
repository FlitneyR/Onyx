#include "SDLInput.h"

namespace onyx
{

SDLInput::SDLInput()
{
}

SDLInput::~SDLInput()
{
	while ( !m_controllers.empty() )
	{
		SDL_GameController* const gc = m_controllers.back();
		SDL_Joystick* const js = SDL_GameControllerGetJoystick( gc );

		m_controllers.pop_back();
	}
}

void SDLInput::ResetMouseState()
{
	SetAxisState( InputAxis::MouseAxis_X, 0.f );
	SetAxisState( InputAxis::MouseAxis_Y, 0.f );
}

void SDLInput::ProcessEvent( const SDL_Event& event )
{
	switch ( event.type )
	{
	case SDL_KEYUP:
	case SDL_KEYDOWN:
		SetAxisState( InputAxisFromKey( event.key.keysym.sym ), event.key.state ? 1.f : 0.f );
		break;
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEBUTTONDOWN:
		SetAxisState( InputAxisFromMouseButton( event.button.button ), event.button.state ? 1.f : 0.f );
		break;
	case SDL_MOUSEMOTION:
		m_mousePos.x = (f32)event.motion.x;
		m_mousePos.y = (f32)event.motion.y;
		SetAxisState( InputAxis::MouseAxis_X, (f32)event.motion.xrel );
		SetAxisState( InputAxis::MouseAxis_Y, (f32)event.motion.yrel );
		break;
	case SDL_CONTROLLERDEVICEADDED:
		AddController( event.cdevice.which );
		break;
	case SDL_CONTROLLERDEVICEREMOVED:
		RemoveController( event.cdevice.which );
		break;
	case SDL_CONTROLLERAXISMOTION:
	{
		const InputAxis axis = GetInputAxisFromControllerAxis( event.caxis.axis );
		f32 value = (f32)event.caxis.value / INT16_MAX;

		if ( axis == InputAxis::GamepadAxis_LeftStickY || axis == InputAxis::GamepadAxis_RightStickY )
			value *= -1.f;

		SetAxisState( axis, value );
		break;
	}
	case SDL_CONTROLLERBUTTONUP:
	case SDL_CONTROLLERBUTTONDOWN:
		SetAxisState( InputAxisFromControllerButton( event.cbutton.button ), event.cbutton.state );
		break;
	default:
		break;
	}
}

void SDLInput::AddController( u8 controller_index )
{
	SDL_GameController* const controller = SDL_GameControllerOpen( controller_index );
	if ( !WEAK_ASSERT( controller, "Failed to connect gamepad {}", controller_index ) )
		return;

	m_controllers.push_back( controller );
}

void SDLInput::RemoveController( u8 controller_id )
{
	std::erase_if( m_controllers, [ &controller_id ]( SDL_GameController* controller ) {
		return SDL_JoystickInstanceID( SDL_GameControllerGetJoystick( controller ) ) == controller_id;
	} );
}

InputAxis SDLInput::InputAxisFromKey( SDL_Keycode key )
{
	switch ( key )
	{
	case SDLK_a:			return InputAxis::Keyboard_A;
	case SDLK_b:			return InputAxis::Keyboard_B;
	case SDLK_c:			return InputAxis::Keyboard_C;
	case SDLK_d:			return InputAxis::Keyboard_D;
	case SDLK_e:			return InputAxis::Keyboard_E;
	case SDLK_f:			return InputAxis::Keyboard_F;
	case SDLK_g:			return InputAxis::Keyboard_G;
	case SDLK_h:			return InputAxis::Keyboard_H;
	case SDLK_j:			return InputAxis::Keyboard_I;
	case SDLK_i:			return InputAxis::Keyboard_J;
	case SDLK_k:			return InputAxis::Keyboard_K;
	case SDLK_l:			return InputAxis::Keyboard_L;
	case SDLK_m:			return InputAxis::Keyboard_M;
	case SDLK_n:			return InputAxis::Keyboard_N;
	case SDLK_o:			return InputAxis::Keyboard_O;
	case SDLK_p:			return InputAxis::Keyboard_P;
	case SDLK_q:			return InputAxis::Keyboard_Q;
	case SDLK_r:			return InputAxis::Keyboard_R;
	case SDLK_s:			return InputAxis::Keyboard_S;
	case SDLK_t:			return InputAxis::Keyboard_T;
	case SDLK_u:			return InputAxis::Keyboard_U;
	case SDLK_v:			return InputAxis::Keyboard_V;
	case SDLK_w:			return InputAxis::Keyboard_W;
	case SDLK_x:			return InputAxis::Keyboard_X;
	case SDLK_y:			return InputAxis::Keyboard_Y;
	case SDLK_z:			return InputAxis::Keyboard_Z;
	case SDLK_UP:			return InputAxis::Keyboard_UpArrow;
	case SDLK_DOWN:			return InputAxis::Keyboard_DownArrow;
	case SDLK_LEFT:			return InputAxis::Keyboard_LeftArrow;
	case SDLK_RIGHT:		return InputAxis::Keyboard_RightArrow;
	case SDLK_SPACE:		return InputAxis::Keyboard_Space;
	case SDLK_LALT:			return InputAxis::Keyboard_LeftAlt;
	case SDLK_LCTRL:		return InputAxis::Keyboard_LeftCtrl;
	case SDLK_LSHIFT:		return InputAxis::Keyboard_LeftShift;
	case SDLK_RALT:			return InputAxis::Keyboard_RightAlt;
	case SDLK_RCTRL:		return InputAxis::Keyboard_RightCtrl;
	case SDLK_RSHIFT:		return InputAxis::Keyboard_RightShift;
	case SDLK_TAB:			return InputAxis::Keyboard_Tab;
	case SDLK_CAPSLOCK:		return InputAxis::Keyboard_CapsLock;
	case SDLK_BACKQUOTE:	return InputAxis::Keyboard_BackTick;
	case SDLK_0:			return InputAxis::Keyboard_NumRow0;
	case SDLK_1:			return InputAxis::Keyboard_NumRow1;
	case SDLK_2:			return InputAxis::Keyboard_NumRow2;
	case SDLK_3:			return InputAxis::Keyboard_NumRow3;
	case SDLK_4:			return InputAxis::Keyboard_NumRow4;
	case SDLK_5:			return InputAxis::Keyboard_NumRow5;
	case SDLK_6:			return InputAxis::Keyboard_NumRow6;
	case SDLK_7:			return InputAxis::Keyboard_NumRow7;
	case SDLK_8:			return InputAxis::Keyboard_NumRow8;
	case SDLK_9:			return InputAxis::Keyboard_NumRow9;
	case SDLK_F1:			return InputAxis::Keyboard_F1;
	case SDLK_F2:			return InputAxis::Keyboard_F2;
	case SDLK_F3:			return InputAxis::Keyboard_F3;
	case SDLK_F4:			return InputAxis::Keyboard_F4;
	case SDLK_F5:			return InputAxis::Keyboard_F5;
	case SDLK_F6:			return InputAxis::Keyboard_F6;
	case SDLK_F7:			return InputAxis::Keyboard_F7;
	case SDLK_F8:			return InputAxis::Keyboard_F8;
	case SDLK_F9:			return InputAxis::Keyboard_F9;
	case SDLK_F10:			return InputAxis::Keyboard_F10;
	case SDLK_F11:			return InputAxis::Keyboard_F11;
	case SDLK_F12:			return InputAxis::Keyboard_F12;
	case SDLK_RETURN:		return InputAxis::Keyboard_Return;
	case SDLK_DELETE:		return InputAxis::Keyboard_Delete;
	default:
		return InputAxis::None;
	}
}

InputAxis SDLInput::InputAxisFromMouseButton( u8 button )
{
	switch ( button )
	{
	case 1: return InputAxis::MouseButton_Left;
	case 2: return InputAxis::MouseButton_Middle;
	case 3: return InputAxis::MouseButton_Right;
	default:
		return InputAxis::None;
	}
}

InputAxis SDLInput::InputAxisFromControllerButton( u8 button )
{
	switch ( button )
	{
	case SDL_CONTROLLER_BUTTON_A:				return InputAxis::GamepadButton_FaceDown;
	case SDL_CONTROLLER_BUTTON_B:				return InputAxis::GamepadButton_FaceRight;
	case SDL_CONTROLLER_BUTTON_X:				return InputAxis::GamepadButton_FaceLeft;
	case SDL_CONTROLLER_BUTTON_Y:				return InputAxis::GamepadButton_FaceUp;
	case SDL_CONTROLLER_BUTTON_DPAD_UP:			return InputAxis::GamepadButton_DPadUp;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:		return InputAxis::GamepadButton_DPadDown;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:		return InputAxis::GamepadButton_DPadLeft;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:		return InputAxis::GamepadButton_DPadRight;
	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:	return InputAxis::GamepadButton_LeftShoulder;
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:	return InputAxis::GamepadButton_RightShoulder;
	case SDL_CONTROLLER_BUTTON_LEFTSTICK:		return InputAxis::GamepadButton_LeftStick;
	case SDL_CONTROLLER_BUTTON_RIGHTSTICK:		return InputAxis::GamepadButton_RightStick;
	case SDL_CONTROLLER_BUTTON_START:			return InputAxis::GamepadButton_Start;
	case SDL_CONTROLLER_BUTTON_GUIDE:			return InputAxis::GamepadButton_Select;
	default:
		return InputAxis::None;
	}
}

InputAxis SDLInput::GetInputAxisFromControllerAxis( u8 axis )
{
	switch ( axis )
	{
	case SDL_CONTROLLER_AXIS_LEFTX:			return InputAxis::GamepadAxis_LeftStickX;
	case SDL_CONTROLLER_AXIS_LEFTY:			return InputAxis::GamepadAxis_LeftStickY;
	case SDL_CONTROLLER_AXIS_RIGHTX:		return InputAxis::GamepadAxis_RightStickX;
	case SDL_CONTROLLER_AXIS_RIGHTY:		return InputAxis::GamepadAxis_RightStickY;
	case SDL_CONTROLLER_AXIS_TRIGGERLEFT:	return InputAxis::GamepadAxis_LeftTrigger;
	case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:	return InputAxis::GamepadAxis_RightTrigger;
	default:
		return InputAxis::None;
	}
}

}