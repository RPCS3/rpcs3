#pragma once
#include "Emu/GS/GCM.h"
#include "Emu/GS/RSXThread.h"

wxSize AspectRatio(wxSize rs, const wxSize as);

class GSFrame : public wxFrame
{
protected:
	GSFrame(wxWindow* parent, const wxString& title);

	virtual void SetViewport(int x, int y, u32 w, u32 h) {}
	virtual void OnPaint(wxPaintEvent& event);
	virtual void OnClose(wxCloseEvent& event);

	//virtual void OnSize(wxSizeEvent&);

	void OnKeyDown(wxKeyEvent& event);
	void OnFullScreen();

public:
	void OnLeftDclick(wxMouseEvent&)
	{
		OnFullScreen();
	}

	//void SetSize(int width, int height);

private:
	DECLARE_EVENT_TABLE();
};

struct GSRender : public RSXThread
{
	virtual ~GSRender()
	{
	}

	virtual void Close()=0;
};

enum GSLockType
{
	GS_LOCK_NOT_WAIT,
	GS_LOCK_WAIT_FLUSH,
	GS_LOCK_WAIT_FLIP,
};

struct GSLock
{
private:
	GSRender& m_renderer;
	GSLockType m_type;

public:
	GSLock(GSRender& renderer, GSLockType type)
		: m_renderer(renderer)
		, m_type(type)
	{
		switch(m_type)
		{
		case GS_LOCK_NOT_WAIT: m_renderer.m_cs_main.Enter(); break;
		case GS_LOCK_WAIT_FLUSH: m_renderer.m_sem_flush.Wait(); break;
		case GS_LOCK_WAIT_FLIP: m_renderer.m_sem_flip.Wait(); break;
		}
	}

	~GSLock()
	{
		switch(m_type)
		{
		case GS_LOCK_NOT_WAIT: m_renderer.m_cs_main.Leave(); break;
		case GS_LOCK_WAIT_FLUSH: m_renderer.m_sem_flush.Post(); break;
		case GS_LOCK_WAIT_FLIP: m_renderer.m_sem_flip.Post(); break;
		}
	}
};

struct GSLockCurrent : GSLock
{
	GSLockCurrent(GSLockType type);
};