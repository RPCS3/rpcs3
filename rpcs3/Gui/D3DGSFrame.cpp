#include "stdafx_gui.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "D3DGSFrame.h"
#include "Utilities/Timer.h"

D3DGSFrame::D3DGSFrame()
	: GSFrame(nullptr, "GSFrame[OpenGL]")
	, m_frames(0)
{
	canvas = new wxWindow(this, wxID_ANY);
	canvas->SetSize(GetClientSize());

	canvas->Bind(wxEVT_LEFT_DCLICK, &GSFrame::OnLeftDclick, this);
}

D3DGSFrame::~D3DGSFrame()
{
}

void D3DGSFrame::Close()
{
	GSFrame::Close();
}

bool D3DGSFrame::IsShown()
{
	return GSFrame::IsShown();
}

void D3DGSFrame::Hide()
{
	GSFrame::Hide();
}

void D3DGSFrame::Show()
{
	GSFrame::Show();
}

void* D3DGSFrame::GetNewContext()
{
	return nullptr;//new wxGLContext(GetCanvas());
}

void D3DGSFrame::SetCurrent(void* ctx)
{
//	GetCanvas()->SetCurrent(*(wxGLContext*)ctx);
}

void D3DGSFrame::DeleteContext(void* ctx)
{
//	delete (wxGLContext*)ctx;
}

void D3DGSFrame::Flip(void* context)
{
	if (!canvas) return;
//	canvas->SetCurrent(*(wxGLContext*)context);

	static Timer fps_t;
//	canvas->SwapBuffers();
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

void D3DGSFrame::OnSize(wxSizeEvent& event)
{
	if (canvas) canvas->SetSize(GetClientSize());
	event.Skip();
}

void D3DGSFrame::SetViewport(int x, int y, u32 w, u32 h)
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

HWND D3DGSFrame::getHandle() const
{
	return canvas->GetHandle();
}
