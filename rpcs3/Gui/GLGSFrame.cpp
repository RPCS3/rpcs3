#include "stdafx.h"
#include "stdafx_gui.h"
#include "Utilities/Config.h"
#include "GLGSFrame.h"
#include <wx/version.h>

extern cfg::bool_entry g_cfg_rsx_debug_output;

GLGSFrame::GLGSFrame(int w, int h)
	: GSFrame("OpenGL", w, h)
{
	const int context_attrs[] =
	{
		WX_GL_RGBA,
		WX_GL_DEPTH_SIZE, 16,
		WX_GL_DOUBLEBUFFER,
#if wxCHECK_VERSION(3, 1, 0)
		WX_GL_MAJOR_VERSION, 3,
		WX_GL_MINOR_VERSION, 3,
		WX_GL_CORE_PROFILE,
#if !defined(CMAKE_BUILD)
		g_cfg_rsx_debug_output ? WX_GL_DEBUG : 0,
#endif
#endif
		0
	};

	m_canvas = new wxGLCanvas(this, wxID_ANY, context_attrs, wxDefaultPosition, { w, h });
	m_canvas->Bind(wxEVT_LEFT_DCLICK, &GSFrame::OnLeftDclick, this);
}

void* GLGSFrame::make_context()
{
	return new wxGLContext(m_canvas);
}

void GLGSFrame::set_current(draw_context_t ctx)
{
	m_canvas->SetCurrent(*(wxGLContext*)ctx.get());
}

void GLGSFrame::delete_context(void* ctx)
{
	delete (wxGLContext*)ctx;
}

void GLGSFrame::flip(draw_context_t context)
{
	GSFrame::flip(context);

	if (!m_canvas) return;

	m_canvas->SetCurrent(*(wxGLContext*)context.get());
	m_canvas->SwapBuffers();
}

void GLGSFrame::OnSize(wxSizeEvent& event)
{
	if (m_canvas)
		m_canvas->SetSize(GetClientSize());

	event.Skip();
}
