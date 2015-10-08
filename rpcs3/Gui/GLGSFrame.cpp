#include "stdafx_gui.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "GLGSFrame.h"
#include "Utilities/Timer.h"

#ifndef _WIN32
#include "frame_icon.xpm"
#endif

GLGSFrame::GLGSFrame() : GSFrame("OpenGL")
{
	SetIcon(wxICON(frame_icon));
	m_canvas = new wxGLCanvas(this, wxID_ANY, NULL);
	m_canvas->SetSize(GetClientSize());

	m_canvas->Bind(wxEVT_LEFT_DCLICK, &GSFrame::OnLeftDclick, this);
}

GLGSFrame::~GLGSFrame()
{
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