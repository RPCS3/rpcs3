#include "stdafx_gui.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "GLGSFrame.h"
#include "Utilities/Timer.h"

GLGSFrame::GLGSFrame()
	: GSFrame(nullptr, "GSFrame[OpenGL]")
	, m_frames(0)
{
	canvas = new wxGLCanvas(this, wxID_ANY, NULL);
	canvas->SetSize(GetClientSize());

	canvas->Bind(wxEVT_LEFT_DCLICK, &GSFrame::OnLeftDclick, this);
}

GLGSFrame::~GLGSFrame()
{
}

void GLGSFrame::Close()
{
	GSFrame::Close();
}

bool GLGSFrame::IsShown()
{
	return GSFrame::IsShown();
}

void GLGSFrame::Hide()
{
	GSFrame::Hide();
}

void GLGSFrame::Show()
{
	GSFrame::Show();
}

void* GLGSFrame::GetNewContext()
{
	return new wxGLContext(GetCanvas());
}

void GLGSFrame::SetCurrent(void* ctx)
{
	GetCanvas()->SetCurrent(*(wxGLContext*)ctx);
}

void GLGSFrame::DeleteContext(void* ctx)
{
	delete (wxGLContext*)ctx;
}

void GLGSFrame::Flip(void* context)
{
	if (!canvas) return;
	canvas->SetCurrent(*(wxGLContext*)context);

	static Timer fps_t;
	canvas->SwapBuffers();
	m_frames++;

	const std::string sub_title = Emu.GetTitle() += Emu.GetTitleID().length() ? " [" + Emu.GetTitleID() + "] | " : " | ";

	if (fps_t.GetElapsedTimeInSec() >= 0.5)
	{
		// can freeze on exit
		SetTitle(sub_title + wxString::Format("FPS: %.2f", (double)m_frames / fps_t.GetElapsedTimeInSec()));
		m_frames = 0;
		fps_t.Start();
	}
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