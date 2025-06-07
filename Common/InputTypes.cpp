#define KEEP_INPUT_AXES_XMACRO
#include "InputTypes.h"

namespace onyx
{

const char* GetInputAxisName( InputAxis axis )
{
	static const char* const names[] { INPUT_AXES( INPUT_AXIS_DESCRIPTION ) };
	return axis >= InputAxis::Count ? nullptr : names[ (u32)axis ];
}

const char* GetInputDeviceName( InputDevice device )
{
	static const char* const names[] { INPUT_DEVICES( INPUT_DEVICE_NAME ) };
	return device >= InputDevice::Count ? nullptr : names[ (u32)device ];
}

InputDevice GetInputDeviceForAxis( InputAxis axis )
{
	static const InputDevice devices[] { INPUT_AXES( INPUT_AXIS_DEVICE ) };
	return axis >= InputAxis::Count ? InputDevice::None : devices[ (u32)axis ];
}

}
