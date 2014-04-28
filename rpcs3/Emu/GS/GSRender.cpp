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
	CellVideoOutResolution res = ResolutionTable[ResolutionIdToNum(Ini.GSResolution.GetValue())];
	SetClientSize(res.width, res.height);
	wxGetApp().Bind(wxEVT_KEY_DOWN, &GSFrame::OnKeyDown, this);
	Bind(wxEVT_CLOSE_WINDOW, &GSFrame::OnClose, this);
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
	case WXK_RETURN: if(event.AltDown()) { OnFullScreen(); return; } break;
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

GSLockCurrent::GSLockCurrent(GSLockType type) : GSLock(Emu.GetGSManager().GetRender(), type)
{
}
