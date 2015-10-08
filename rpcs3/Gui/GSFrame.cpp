#include "stdafx_gui.h"
#include "GSFrame.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules/cellVideoOut.h"
#include "rpcs3.h"

BEGIN_EVENT_TABLE(GSFrame, wxFrame)
	EVT_PAINT(GSFrame::OnPaint)
	EVT_SIZE(GSFrame::OnSize)
END_EVENT_TABLE()

GSFrame::GSFrame(const wxString& title) : wxFrame(nullptr, wxID_ANY, title)
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

void GSFrame::flip(draw_context_t)
{
	++m_frames;

	static Timer fps_t;

	if (fps_t.GetElapsedTimeInSec() >= 0.5)
	{
		std::string title = fmt::format("FPS: %.2f", (double)m_frames / fps_t.GetElapsedTimeInSec());

		if (!m_title_message.empty())
			title += " | " + m_title_message;

		if (!Emu.GetTitle().empty())
			title += " | " + Emu.GetTitle();

		if (!Emu.GetTitleID().empty())
			title += " | [" + Emu.GetTitleID() + "]";

		// can freeze on exit
		SetTitle(wxString(title.c_str(), wxConvUTF8));
		m_frames = 0;
		fps_t.Start();
	}
}