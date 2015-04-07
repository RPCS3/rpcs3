#include "stdafx_gui.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "D3D9GSFrame.h"
#include "Utilities/Timer.h"

D3D9GSFrame::D3D9GSFrame()
	: GSFrame(nullptr, "GSFrame[Direct3D 9]")
	, m_frames(0)
{
}

D3D9GSFrame::~D3D9GSFrame()
{
}

void D3D9GSFrame::Close()
{
	GSFrame::Close();
}

bool D3D9GSFrame::IsShown()
{
	return GSFrame::IsShown();
}

void D3D9GSFrame::Hide()
{
	GSFrame::Hide();
}

void D3D9GSFrame::Show()
{
	GSFrame::Show();
}

void* D3D9GSFrame::GetNewContext()
{
	return GSFrame::GetHWND();
	//return nullptr;
}

void D3D9GSFrame::SetCurrent(void* ctx)
{
}

void D3D9GSFrame::DeleteContext(void* ctx)
{
}

void D3D9GSFrame::Flip(void* context)
{
	static Timer fps_t;
	m_frames++;

	const std::string title = (Emu.GetTitleID().empty() ? "" : " [" + Emu.GetTitleID() + "]");

	if (fps_t.GetElapsedTimeInSec() >= 0.5)
	{
		// can freeze on exit
		SetTitle(title + wxString::Format("FPS: %.2f | %s", (double)m_frames / fps_t.GetElapsedTimeInSec(), title.c_str()));
		m_frames = 0;
		fps_t.Start();
	}
}

sizei D3D9GSFrame::GetClientSize()
{
	return{};
	//return nullptr;
}

void D3D9GSFrame::OnSize(wxSizeEvent& event)
{
	event.Skip();
}

void D3D9GSFrame::SetViewport(int x, int y, u32 w, u32 h)
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