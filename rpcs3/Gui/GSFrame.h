#pragma once 

#include "Emu/RSX/GSRender.h"

class GSFrame : public wxFrame, public GSFrameBase
{
	u64 m_frames = 0;
	std::string m_render;

public:
	GSFrame(const wxString& title, int w, int h);

protected:
	virtual void OnPaint(wxPaintEvent& event);
	virtual void OnClose(wxCloseEvent& event);

	void OnKeyDown(wxKeyEvent& event);
	void OnFullScreen();

	void close() override;

	bool shown() override;
	void hide() override;
	void show() override;

	void* handle() const override;

	void* make_context() override;
	void set_current(draw_context_t context) override;
	void delete_context(void* context) override;
	void flip(draw_context_t context) override;
	int client_width() override;
	int client_height() override;

public:
	void OnLeftDclick(wxMouseEvent&)
	{
		OnFullScreen();
	}

	//void SetSize(int width, int height);

private:
	DECLARE_EVENT_TABLE();
};
