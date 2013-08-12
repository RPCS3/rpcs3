#pragma once
#include "Emu/GS/GSRender.h"

struct NullGSFrame : public GSFrame
{
	NullGSFrame() : GSFrame(NULL, "GSFrame[Null]")
	{
		Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(GSFrame::OnLeftDclick));
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

struct NullRSXThread : public wxThread
{
	wxWindow* m_parent;
	Stack<u32> call_stack;

	NullRSXThread(wxWindow* parent);

	virtual void OnExit();
	void Start();
	ExitCode Entry();
};

class NullGSRender
	: public wxWindow
	, public GSRender
{
private:
	NullRSXThread* m_rsx_thread;

public:
	NullGSFrame* m_frame;

	NullGSRender()
		: m_frame(nullptr)
		, m_rsx_thread(nullptr)
	{
		m_draw = false;
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

		(m_rsx_thread = new NullRSXThread(this))->Start();
	}

public:
	void DoCmd(const u32 fcmd, const u32 cmd, mem32_t& args, const u32 count)
	{
		switch(cmd)
		{
		case NV406E_SET_REFERENCE:
			m_ctrl->ref = re32(args[0]);
		break;
		}
	}

	virtual void Draw()
	{
		//if(m_frame && !m_frame->IsBeingDeleted()) m_frame->Draw();
		m_draw = true;
	}

	virtual void Close()
	{
		if(m_rsx_thread) m_rsx_thread->Delete();
		if(m_frame->IsShown()) m_frame->Hide();
		m_ctrl = NULL;
	}
};

NullRSXThread::NullRSXThread(wxWindow* parent)
	: wxThread(wxTHREAD_DETACHED)
	, m_parent(parent)
{
}

void NullRSXThread::OnExit()
{
	call_stack.Clear();
}

void NullRSXThread::Start()
{
	Create();
	Run();
}

wxThread::ExitCode NullRSXThread::Entry()
{
	ConLog.Write("Null RSX thread entry");

	NullGSRender& p = *(NullGSRender*)m_parent;

	while(!TestDestroy() && p.m_frame && !p.m_frame->IsBeingDeleted())
	{
		wxCriticalSectionLocker lock(p.m_cs_main);

		if(p.m_ctrl->get == p.m_ctrl->put || !Emu.IsRunning())
		{
			SemaphorePostAndWait(p.m_sem_flush);

			if(p.m_draw)
			{
				p.m_draw = false;
				p.m_flip_status = 0;
				if(SemaphorePostAndWait(p.m_sem_flip)) continue;
			}

			Sleep(1);
			continue;
		}

		const u32 get = re(p.m_ctrl->get);
		const u32 cmd = Memory.Read32(p.m_ioAddress + get);
		const u32 count = (cmd >> 18) & 0x7ff;

		if(cmd & CELL_GCM_METHOD_FLAG_JUMP)
		{
			p.m_ctrl->get = re32(cmd & ~(CELL_GCM_METHOD_FLAG_JUMP | CELL_GCM_METHOD_FLAG_NON_INCREMENT));
			ConLog.Warning("rsx jump!");
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_CALL)
		{
			call_stack.Push(get + 4);
			p.m_ctrl->get = re32(cmd & ~CELL_GCM_METHOD_FLAG_CALL);
			ConLog.Warning("rsx call!");
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_RETURN)
		{
			p.m_ctrl->get = re32(call_stack.Pop());
			ConLog.Warning("rsx return!");
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_NON_INCREMENT)
		{
			//ConLog.Warning("non increment cmd! 0x%x", cmd);
		}

		if(cmd == 0)
		{
			ConLog.Warning("null cmd: addr=0x%x, put=0x%x, get=0x%x", p.m_ioAddress + get, re(p.m_ctrl->put), get);
			Emu.Pause();
			continue;
		}

		p.DoCmd(cmd, cmd & 0x3ffff, mem32_t(p.m_ioAddress + get + 4), count);
		re(p.m_ctrl->get, get + (count + 1) * 4);
	}

	ConLog.Write("Null RSX thread exit...");

	call_stack.Clear();

	return (ExitCode)0;
}