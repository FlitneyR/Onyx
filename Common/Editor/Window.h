#pragma once

#include "imgui.h"

namespace onyx::editor
{

struct IWindow
{
	// override this with the name of your window class
	inline static const char* const s_name = "Unnamed Window";
	virtual const char* GetName() const { return s_name; }
	bool m_open = true;
	virtual void Run() = 0;
};

void DoWindowsMenu();
void DoWindows();

}
