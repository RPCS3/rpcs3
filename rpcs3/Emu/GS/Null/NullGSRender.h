#pragma once
#include "Emu/GS/GSRender.h"

struct NullGSFrame : public GSFrame
{
	NullGSFrame() : GSFrame(NULL, "GSFrame[Null]")
	{
	}

	void Draw() { Draw(wxClientDC(this)); }

private:
	virtual void OnPaint(wxPaintEvent& event) { Draw(wxPaintDC(this)); }
	virtual void OnSize(wxSizeEvent& event)
	{
		GSFrame::OnSize(event);
		Draw();
	}

	void Draw(wxDC& dc)
	{
		dc.DrawText("Null GS output", 0, 0);
	}
};

class NullGSRender
	: public wxWindow
	, public GSRender
{
private:
	NullGSFrame* m_frame;
	wxTimer* m_update_timer;
	bool m_paused;

public:
	NullGSRender()
		: m_frame(NULL)
		, m_paused(false)
	{
		m_update_timer = new wxTimer(this);
		Connect(m_update_timer->GetId(), wxEVT_TIMER, wxTimerEventHandler(NullGSRender::OnTimer));
		m_frame = new NullGSFrame();
	}

	~NullGSRender()
	{
		Close();
		m_frame->Close();
	}

private:
	virtual void Init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress)
	{
		if(m_frame->IsShown()) return;
		
		m_frame->SetSize(740, 480);
		m_frame->Show();

		m_ioAddress = ioAddress;
		m_ctrlAddress = ctrlAddress;
		m_ioSize = ioSize;
		m_localAddress = localAddress;
		m_ctrl = (CellGcmControl*)Memory.GetMemFromAddr(m_ctrlAddress);

		m_update_timer->Start(1);
	}

	void OnTimer(wxTimerEvent&)
	{
		while(!m_paused && m_ctrl->get != m_ctrl->put && Emu.IsRunned())
		{
			const u32 get = re(m_ctrl->get);
			const u32 cmd = Memory.Read32(m_ioAddress + get);
			const u32 count = (cmd >> 18) & 0x7ff;
			mem32_t data(m_ioAddress + get + 4);

			m_ctrl->get = re32(get + (count + 1) * 4);
			DoCmd(cmd, cmd & 0x3ffff, data, count);
			memset(Memory.GetMemFromAddr(m_ioAddress + get), 0, (count + 1) * 4);
		}
	}

	void DoCmd(const u32 fcmd, const u32 cmd, mem32_t& args, const u32 count)
	{
		switch(cmd)
		{
		case NV406E_SET_REFERENCE:
			m_ctrl->ref = re32(args[0]);
		break;

		case NV4097_SET_BEGIN_END:
			if(!args[0]) m_flip_status = 0;
		break;
		}
	}

	virtual void Draw()
	{
		//if(m_frame && !m_frame->IsBeingDeleted()) m_frame->Draw();
	}

	virtual void Close()
	{
		m_update_timer->Stop();
		if(m_frame->IsShown()) m_frame->Hide();
		m_ctrl = NULL;
	}

	void Pause()
	{
		m_paused = true;
	}

	void Resume()
	{
		m_paused = false;
	}
};