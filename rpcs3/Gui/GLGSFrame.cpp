#include "stdafx.h"
#include "stdafx_gui.h"
#include "GLGSFrame.h"

GLGSFrame::GLGSFrame() : GSFrame("OpenGL")
{
	// For some reason WX_GL_MAJOR_VERSION, WX_GL_MINOR_VERSION and WX_GL_CORE_PROFILE are not defined on travis build despite using wxWidget 3.0+
#ifdef WIN32
	const int attributes[] =
	{
		WX_GL_RGBA,
		WX_GL_DOUBLEBUFFER,
		WX_GL_MAJOR_VERSION, 3,
		WX_GL_MINOR_VERSION, 3,
		WX_GL_CORE_PROFILE,
		0
	};
	m_canvas = new wxGLCanvas(this, wxID_ANY, attributes);
#else
	m_canvas = new wxGLCanvas(this, wxID_ANY, NULL);
#endif
	m_canvas->SetSize(GetClientSize());

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
