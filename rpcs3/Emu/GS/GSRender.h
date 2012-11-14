#pragma once
#include "Emu/GS/GCM.h"

wxSize AspectRatio(wxSize rs, const wxSize as);

class GSFrame : public wxFrame
{
protected:
	GSFrame(wxWindow* parent, const wxString& title);

	virtual void SetViewport(int x, int y, u32 w, u32 h) {}
	virtual void OnPaint(wxPaintEvent& event);
	virtual void OnClose(wxCloseEvent& event);

	//virtual void OnSize(wxSizeEvent&);

	void OnLeftDclick(wxMouseEvent&)
	{
		OnFullScreen();
	}

	void OnKeyDown(wxKeyEvent& event);
	void OnFullScreen();

public:
	//void SetSize(int width, int height);

private:
	DECLARE_EVENT_TABLE();
};

struct GSRender
{
	u32 m_ioAddress, m_ioSize, m_ctrlAddress, m_localAddress;
	CellGcmControl* m_ctrl;
	int m_flip_status;

	GSRender();

	virtual void Init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress)=0;
	virtual void Draw()=0;
	virtual void Close()=0;
	virtual void Pause()=0;
	virtual void Resume()=0;

	u32 GetAddress(u32 offset, u8 location);
};