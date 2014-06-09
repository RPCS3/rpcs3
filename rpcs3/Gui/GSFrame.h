#pragma once 

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