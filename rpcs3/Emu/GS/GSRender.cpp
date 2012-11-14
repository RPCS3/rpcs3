#include "stdafx.h"
#include "GSRender.h"

wxSize AspectRatio(wxSize rs, const wxSize as)
{
	const double aq = (double)as.x / as.y;
	const double rq = (double)rs.x / rs.y;
	const double q = aq / rq;

	if(q > 1.0)
	{
		rs.y /= q;
	}
	else if(q < 1.0)
	{
		rs.x *= q;
	}

	return rs;
}

GSFrame::GSFrame(wxWindow* parent, const wxString& title) : wxFrame(parent, wxID_ANY, title)
{
	SetClientSize(720, 576);
	//Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(GSFrame::OnLeftDclick));
	wxGetApp().Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(GSFrame::OnKeyDown), (wxObject*)0, this);
	wxGetApp().Connect(GetId(), wxEVT_CLOSE_WINDOW, wxCloseEventHandler(GSFrame::OnClose), (wxObject*)0, this);
}

void GSFrame::OnPaint(wxPaintEvent& event)
{
	wxPaintDC(this);
}

void GSFrame::OnClose(wxCloseEvent& event)
{
	Emu.Stop();
}

/*
void GSFrame::OnSize(wxSizeEvent&)
{
	const wxSize client = GetClientSize();
	const wxSize viewport = AspectRatio(client, m_size);

	const int x = (client.GetX() - viewport.GetX()) / 2;
	const int y = (client.GetY() - viewport.GetY()) / 2;

	SetViewport(wxPoint(x, y), viewport);
}
*/

void GSFrame::OnKeyDown(wxKeyEvent& event)
{
	switch(event.GetKeyCode())
	{
	case WXK_RETURN: if(event.ControlDown()) { OnFullScreen(); return; } break;
	case WXK_ESCAPE: if(IsFullScreen()) { ShowFullScreen(false); return; } break;
	}
	event.Skip();
}

void GSFrame::OnFullScreen()
{
	ShowFullScreen(!IsFullScreen());
}


/*
void GSFrame::SetSize(int width, int height)
{
	m_size.SetWidth(width);
	m_size.SetHeight(height);
	//wxFrame::SetSize(width, height);
	OnSize(wxSizeEvent());
}
*/


GSRender::GSRender()
	: m_ctrl(NULL)
	, m_flip_status(-1)
{
}

u32 GSRender::GetAddress(u32 offset, u8 location)
{
	switch(location)
	{
	case CELL_GCM_LOCATION_LOCAL: return Memory.RSXFBMem.GetStartAddr() + offset;
	case CELL_GCM_LOCATION_MAIN: return offset;
	}
	return 0;
}