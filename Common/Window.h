#pragma once
#include "Common/Graphics/DeleteQueue.h"

namespace onyx
{

struct IFrameContext;

struct IWindow;

// for graphics API specific context data and callbacks
struct IWindowContext
{
	IWindow* m_window;
	DeleteQueue m_deleteQueue;

	IWindowContext( IWindow& window ) : m_window( &window ) {}
	virtual ~IWindowContext() { m_deleteQueue.Execute(); }

	virtual IFrameContext& GetFrameContext() = 0;
};

struct IWindow
{
	// todo! add has focus functions
	virtual bool HasClosed() const = 0;
	virtual void Close() = 0;
	virtual glm::uvec2 GetSize() const = 0;
	virtual void SetSize( const glm::uvec2& new_size ) = 0;

	IWindowContext& GetWindowContext() { return *m_windowContext; }

protected:

	std::unique_ptr< IWindowContext > m_windowContext;
};

}
