#pragma once
#include "Window.h"
#include "Common/Scripting/Script.h"

namespace onyx::editor
{

struct ScriptEditorWindow : onyx::editor::IWindow
{
	std::shared_ptr< Script > m_script;


};

}
