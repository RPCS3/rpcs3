#pragma once
#include "Emu/GS/GSRender.h"

struct NullGSFrame : public GSFrame
{
	NullGSFrame() : GSFrame(nullptr, "GSFrame[Null]")
	{
		Bind(wxEVT_LEFT_DCLICK, &GSFrame::OnLeftDclick, this);
	}

	void Draw()
	{
		wxClientDC dc(this);
		Draw(&dc);
	}

private:
	virtual void OnPaint(wxPaintEvent& event)
	{
		wxPaintDC dc(this);
		Draw(&dc);
	}
	virtual void OnSize(wxSizeEvent& event)
	{
		GSFrame::OnSize(event);
		Draw();
	}

	void Draw(wxDC* dc)
	{
		dc->DrawText("Null GS output", 0, 0);
	}
};

class NullGSRender
	: public wxWindow
	, public GSRender
{
public:
	NullGSFrame* m_frame;

	NullGSRender() : m_frame(nullptr)
	{
		m_frame = new NullGSFrame();
	}

	virtual ~NullGSRender()
	{
		m_frame->Close();
	}

private:
	virtual void OnInit()
	{
		m_frame->Show();
	}

	virtual void OnInitThread()
	{
	}

	virtual void OnExitThread()
	{
	}

	virtual void OnReset()
	{
	}

	virtual void ExecCMD()
	{
	}

	virtual void Flip()
	{
	}

	virtual void Close()
	{
		Stop();

		if(m_frame->IsShown()) m_frame->Hide();
	}
};
