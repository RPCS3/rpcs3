#include "stdafx.h"
#include "stdafx_gui.h"

#include "Utilities/Timer.h"
#include "Emu/System.h"
#include "rpcs3.h"

#include "GSFrame.h"

BEGIN_EVENT_TABLE(GSFrame, wxFrame)
	EVT_PAINT(GSFrame::OnPaint)
	EVT_SIZE(GSFrame::OnSize)
END_EVENT_TABLE()

GSFrame::GSFrame(const wxString& title, int w, int h)
	: wxFrame(nullptr, wxID_ANY, "GSFrame[" + title + "]")
{
	m_render = title;
	SetIcon(wxGetApp().m_MainFrame->GetIcon());
	SetCursor(wxCursor(wxImage(1, 1)));

	SetClientSize(w, h);
	wxGetApp().Bind(wxEVT_KEY_DOWN, &GSFrame::OnKeyDown, this);
	Bind(wxEVT_CLOSE_WINDOW, &GSFrame::OnClose, this);
	Bind(wxEVT_LEFT_DCLICK, &GSFrame::OnLeftDclick, this);
}

void GSFrame::OnPaint(wxPaintEvent& event)
{
	wxPaintDC(this);
}

void GSFrame::OnClose(wxCloseEvent& event)
{
	Emu.Stop();
}

void GSFrame::OnKeyDown(wxKeyEvent& event)
{
	switch (event.GetKeyCode())
	{
	case WXK_RETURN: if (event.AltDown()) { OnFullScreen(); return; } break;
	case WXK_ESCAPE: if (IsFullScreen()) { ShowFullScreen(false); return; } break;
	}
	event.Skip();
}

void GSFrame::OnFullScreen()
{
	ShowFullScreen(!IsFullScreen());
}

void GSFrame::close()
{
	wxFrame::Close();
}

bool GSFrame::shown()
{
	return wxFrame::IsShown();
}

void GSFrame::hide()
{
	wxFrame::Hide();
}

void GSFrame::show()
{
	wxFrame::Show();
}

void* GSFrame::handle() const
{
	return GetHandle();
}

void* GSFrame::make_context()
{
	return nullptr;
}

void GSFrame::set_current(draw_context_t ctx)
{
}

void GSFrame::delete_context(void* ctx)
{
}

int GSFrame::client_width()
{
	return GetClientSize().GetWidth();
}

int GSFrame::client_height()
{
	return GetClientSize().GetHeight();
}

void GSFrame::flip(draw_context_t)
{
	++m_frames;

	static Timer fps_t;

	if (fps_t.GetElapsedTimeInSec() >= 0.5)
	{
		std::string title = fmt::format("FPS: %.2f", (double)m_frames / fps_t.GetElapsedTimeInSec());

		if (!m_render.empty())
			title += " | " + m_render;
			
		if (!Emu.GetTitle().empty())
			title += " | " + Emu.GetTitle();

		if (!Emu.GetTitleID().empty())
			title += " | [" + Emu.GetTitleID() + ']';

		wxGetApp().CallAfter([this, title = std::move(title)]
		{
			SetTitle(wxString(title.c_str(), wxConvUTF8));
		});

		m_frames = 0;
		fps_t.Start();
	}
}
