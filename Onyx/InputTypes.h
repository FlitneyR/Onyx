#pragma once

namespace onyx
{

#define INPUT_DEVICES( f ) \
	f( None		)	\
	f( Keyboard	)	\
	f( Mouse	)	\
	f( Gamepad	)	\

#define INPUT_AXES( f ) \
	f( None							, "None"					, None		) \
	f( Keyboard_A					, "A"						, Keyboard	) \
	f( Keyboard_B					, "B"						, Keyboard	) \
	f( Keyboard_C					, "C"						, Keyboard	) \
	f( Keyboard_D					, "D"						, Keyboard	) \
	f( Keyboard_E					, "E"						, Keyboard	) \
	f( Keyboard_F					, "F"						, Keyboard	) \
	f( Keyboard_G					, "G"						, Keyboard	) \
	f( Keyboard_H					, "H"						, Keyboard	) \
	f( Keyboard_I					, "I"						, Keyboard	) \
	f( Keyboard_J					, "J"						, Keyboard	) \
	f( Keyboard_K					, "K"						, Keyboard	) \
	f( Keyboard_L					, "L"						, Keyboard	) \
	f( Keyboard_M					, "M"						, Keyboard	) \
	f( Keyboard_N					, "N"						, Keyboard	) \
	f( Keyboard_O					, "O"						, Keyboard	) \
	f( Keyboard_P					, "P"						, Keyboard	) \
	f( Keyboard_Q					, "Q"						, Keyboard	) \
	f( Keyboard_R					, "R"						, Keyboard	) \
	f( Keyboard_S					, "S"						, Keyboard	) \
	f( Keyboard_T					, "T"						, Keyboard	) \
	f( Keyboard_U					, "U"						, Keyboard	) \
	f( Keyboard_V					, "V"						, Keyboard	) \
	f( Keyboard_W					, "W"						, Keyboard	) \
	f( Keyboard_X					, "X"						, Keyboard	) \
	f( Keyboard_Y					, "Y"						, Keyboard	) \
	f( Keyboard_Z					, "Z"						, Keyboard	) \
	f( Keyboard_UpArrow				, "Up Arrow"				, Keyboard	) \
	f( Keyboard_DownArrow			, "Down Arrow"				, Keyboard	) \
	f( Keyboard_LeftArrow			, "Left Arrow"				, Keyboard	) \
	f( Keyboard_RightArrow			, "Right Arrow"				, Keyboard	) \
	f( Keyboard_Space				, "Space"					, Keyboard	) \
	f( Keyboard_LeftAlt				, "Left Alt"				, Keyboard	) \
	f( Keyboard_LeftCtrl			, "Left Ctrl"				, Keyboard	) \
	f( Keyboard_LeftShift			, "Left Shift"				, Keyboard	) \
	f( Keyboard_RightAlt			, "Right Alt"				, Keyboard	) \
	f( Keyboard_RightCtrl			, "Right Ctrl"				, Keyboard	) \
	f( Keyboard_RightShift			, "Right Shift"				, Keyboard	) \
	f( Keyboard_Tab					, "Tab"						, Keyboard	) \
	f( Keyboard_CapsLock			, "Caps Lock"				, Keyboard	) \
	f( Keyboard_BackTick			, "Back Tick"				, Keyboard	) \
	f( Keyboard_NumRow0				, "0"						, Keyboard	) \
	f( Keyboard_NumRow1				, "1"						, Keyboard	) \
	f( Keyboard_NumRow2				, "2"						, Keyboard	) \
	f( Keyboard_NumRow3				, "3"						, Keyboard	) \
	f( Keyboard_NumRow4				, "4"						, Keyboard	) \
	f( Keyboard_NumRow5				, "5"						, Keyboard	) \
	f( Keyboard_NumRow6				, "6"						, Keyboard	) \
	f( Keyboard_NumRow7				, "7"						, Keyboard	) \
	f( Keyboard_NumRow8				, "8"						, Keyboard	) \
	f( Keyboard_NumRow9				, "9"						, Keyboard	) \
	f( Keyboard_F1					, "F1"						, Keyboard	) \
	f( Keyboard_F2					, "F2"						, Keyboard	) \
	f( Keyboard_F3					, "F3"						, Keyboard	) \
	f( Keyboard_F4					, "F4"						, Keyboard	) \
	f( Keyboard_F5					, "F5"						, Keyboard	) \
	f( Keyboard_F6					, "F6"						, Keyboard	) \
	f( Keyboard_F7					, "F7"						, Keyboard	) \
	f( Keyboard_F8					, "F8"						, Keyboard	) \
	f( Keyboard_F9					, "F9"						, Keyboard	) \
	f( Keyboard_F10					, "F10"						, Keyboard	) \
	f( Keyboard_F11					, "F11"						, Keyboard	) \
	f( Keyboard_F12					, "F12"						, Keyboard	) \
	f( Keyboard_Return				, "Return"					, Keyboard	) \
	f( Keyboard_Delete				, "Delete"					, Keyboard	) \
	f( MouseButton_Left				, "Left Click"				, Mouse		) \
	f( MouseButton_Middle			, "Middle Click"			, Mouse		) \
	f( MouseButton_Right			, "Right Click"				, Mouse		) \
	f( MouseAxis_XY					, "Mouse Move"				, Mouse		) \
	f( MouseAxis_X					, "Mouse Horizontal"		, Mouse		) \
	f( MouseAxis_Y					, "Mouse Vertical"			, Mouse		) \
	f( MouseAxis_Up					, "Mouse Up"				, Mouse		) \
	f( MouseAxis_Down				, "Mouse Down"				, Mouse		) \
	f( MouseAxis_Left				, "Mouse Left"				, Mouse		) \
	f( MouseAxis_Right				, "Mouse Right"				, Mouse		) \
	f( GamepadButton_FaceUp			, "Top Face Button"			, Gamepad	) \
	f( GamepadButton_FaceDown		, "Bottom Face Button"		, Gamepad	) \
	f( GamepadButton_FaceLeft		, "Left Face Button"		, Gamepad	) \
	f( GamepadButton_FaceRight		, "Right Face Button"		, Gamepad	) \
	f( GamepadButton_DPadCombined	, "DPad"					, Gamepad	) \
	f( GamepadButton_DPadUp			, "DPad Up"					, Gamepad	) \
	f( GamepadButton_DPadDown		, "DPad Down"				, Gamepad	) \
	f( GamepadButton_DPadLeft		, "DPad Left"				, Gamepad	) \
	f( GamepadButton_DPadRight		, "DPad Right"				, Gamepad	) \
	f( GamepadButton_Start			, "Start"					, Gamepad	) \
	f( GamepadButton_Select			, "Select"					, Gamepad	) \
	f( GamepadButton_LeftStick		, "Left Stick Click"		, Gamepad	) \
	f( GamepadButton_RightStick		, "Right Stick Click"		, Gamepad	) \
	f( GamepadButton_LeftShoulder	, "Left Shoulder"			, Gamepad	) \
	f( GamepadButton_RightShoulder	, "Right Shoulder"			, Gamepad	) \
	f( GamepadAxis_LeftStickXY		, "Left Stick"				, Gamepad	) \
	f( GamepadAxis_LeftStickX		, "Left Stick Horizontal"	, Gamepad	) \
	f( GamepadAxis_LeftStickY		, "Left Stick Vertical"		, Gamepad	) \
	f( GamepadAxis_LeftStickUp		, "Left Stick Up"			, Gamepad	) \
	f( GamepadAxis_LeftStickDown	, "Left Stick Down"			, Gamepad	) \
	f( GamepadAxis_LeftStickLeft	, "Left Stick Left"			, Gamepad	) \
	f( GamepadAxis_LeftStickRight	, "Left Stick Right"		, Gamepad	) \
	f( GamepadAxis_RightStickXY		, "Right Stick"				, Gamepad	) \
	f( GamepadAxis_RightStickX		, "Right Stick Horizontal"	, Gamepad	) \
	f( GamepadAxis_RightStickY		, "Right Stick Vertical"	, Gamepad	) \
	f( GamepadAxis_RightStickUp		, "Right Stick Up"			, Gamepad	) \
	f( GamepadAxis_RightStickDown	, "Right Stick Down"		, Gamepad	) \
	f( GamepadAxis_RightStickLeft	, "Right Stick Left"		, Gamepad	) \
	f( GamepadAxis_RightStickRight	, "Right Stick Right"		, Gamepad	) \
	f( GamepadAxis_LeftTrigger		, "Left Trigger"			, Gamepad	) \
	f( GamepadAxis_RightTrigger		, "Right Trigger"			, Gamepad	) \

#define INPUT_AXIS_NAME( name, desc, device ) name,
#define INPUT_AXIS_DESCRIPTION( name, desc, device ) desc,
#define INPUT_AXIS_DEVICE( name, desc, device ) InputDevice::device,

enum struct InputAxis : u32
{
	INPUT_AXES( INPUT_AXIS_NAME )
	Count
};

#define INPUT_DEVICE_ENUM( name ) name,
#define INPUT_DEVICE_NAME( name ) #name,

enum struct InputDevice : u8
{
	INPUT_DEVICES( INPUT_DEVICE_ENUM )
	Count,
};

const char* GetInputAxisName( InputAxis axis );
const char* GetInputDeviceName( InputDevice device );
InputDevice GetInputDeviceForAxis( InputAxis axis );

#ifndef KEEP_INPUT_AXES_XMACRO
#undef INPUT_AXES
#undef INPUT_DEVICES

#undef INPUT_AXIS_NAME
#undef INPUT_AXIS_DESCRIPTION
#undef INPUT_AXIS_DEVICE

#undef INPUT_DEVICE_ENUM
#undef INPUT_DEVICE_NAME
#endif

enum struct ButtonState : u8
{
	IsDown = 1,
	WasDown,

	NotHeld = 0,
	Pressed = IsDown & ~WasDown,
	Released = WasDown & ~IsDown,
	Held = IsDown | WasDown,
};

}
