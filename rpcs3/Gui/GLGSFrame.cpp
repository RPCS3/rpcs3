#include "stdafx_gui.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "GLGSFrame.h"
#include "D3DGSFrame.h"
#include "Utilities/Timer.h"

#ifndef _WIN32
#include "frame_icon.xpm"
#endif

GLGSFrame::GLGSFrame()
	: GSFrame(nullptr, "GSFrame[OpenGL]")
	, m_frames(0)
{
	SetIcon(wxICON(frame_icon));
	canvas = new wxGLCanvas(this, wxID_ANY, NULL);
	canvas->SetSize(GetClientSize());

	canvas->Bind(wxEVT_LEFT_DCLICK, &GSFrame::OnLeftDclick, this);
}

void GLGSFrame::close()
{
	GSFrame::Close();
}

bool GLGSFrame::shown()
{
	return GSFrame::IsShown();
}

void GLGSFrame::hide()
{
	GSFrame::Hide();
}

void GLGSFrame::show()
{
	GSFrame::Show();
}

void* GLGSFrame::make_context()
{
	return new wxGLContext(GetCanvas());
}

void GLGSFrame::set_current(draw_context_t ctx)
{
	GetCanvas()->SetCurrent(*(wxGLContext*)ctx.get());
}

void GLGSFrame::delete_context(void* ctx)
{
	delete (wxGLContext*)ctx;
}

void GLGSFrame::flip(draw_context_t context)
{
	if (!canvas) return;
	canvas->SetCurrent(*(wxGLContext*)context.get());

	static Timer fps_t;
	canvas->SwapBuffers();
	m_frames++;

	const std::string sub_title = Emu.GetTitle() + (Emu.GetTitleID().length() ? " [" + Emu.GetTitleID() + "] | " : " | ");

	if (fps_t.GetElapsedTimeInSec() >= 0.5)
	{
		// can freeze on exit
		SetTitle(wxString(sub_title.c_str(), wxConvUTF8) + wxString::Format("FPS: %.2f", (double)m_frames / fps_t.GetElapsedTimeInSec()));
		m_frames = 0;
		fps_t.Start();
	}
}

size2i GLGSFrame::client_size()
{
	wxSize size = GetClientSize();
	return{ size.GetWidth(), size.GetHeight() };
}

void GLGSFrame::OnSize(wxSizeEvent& event)
{
	if (canvas) canvas->SetSize(GetClientSize());
	event.Skip();
}

void GLGSFrame::SetViewport(int x, int y, u32 w, u32 h)
{
	/*
	//ConLog.Warning("SetViewport(x=%d, y=%d, w=%d, h=%d)", x, y, w, h);

	const wxSize client = GetClientSize();
	const wxSize viewport = AspectRatio(client, wxSize(w, h));

	const int vx = (client.GetX() - viewport.GetX()) / 2;
	const int vy = (client.GetY() - viewport.GetY()) / 2;

	glViewport(vx + x, vy + y, viewport.GetWidth(), viewport.GetHeight());
	*/
}