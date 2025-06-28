#pragma once
#include "Common/InputTypes.h"
#include "glm/glm.hpp"

#include "Common/Editor/Window.h"

namespace onyx
{

struct LowLevelInput
{
	LowLevelInput();

	static f32 s_buttonSensitivity;
	static f32 s_deadZone;

	void UpdateButtonStates();

	glm::vec2 GetAxisState( InputAxis axis ) const
	{
		if ( !WEAK_ASSERT( (u32)axis < (u32)InputAxis::Count, "{} is not an input axis", (u32)axis ) )
			return {};

		return m_axisStates[ (u32)axis ];
	}

	ButtonState GetButtonState( InputAxis axis ) const
	{
		if ( !WEAK_ASSERT( (u32)axis < (u32)InputAxis::Count, "{} is not an input axis", (u32)axis ) )
			return {};

		return m_buttonStates[ (u32)axis ];
	}

	glm::vec2 GetMousePos() const { return m_mousePos; }

	struct DebugWindow : editor::IWindow
	{
		inline static const char* const s_name = "Input Debug";
		bool m_inputEnabled[ (u32)onyx::InputAxis::Count ];
		void Run( IFrameContext& frame_context ) override;
		std::string GetWindowTitle() const override;
	};

protected:

	glm::vec2 m_axisStates[ (u32)InputAxis::Count ];
	ButtonState m_buttonStates[ (u32)InputAxis::Count ];
	glm::vec2 m_mousePos = {};

	void SetAxisState( InputAxis axis, f32 state );
};

}
