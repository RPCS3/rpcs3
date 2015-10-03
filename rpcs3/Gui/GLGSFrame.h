#pragma once
#include "Emu/RSX/GL/GLGSRender.h"
#include "Gui/GSFrame.h"
#include "wx/glcanvas.h"

struct GLGSFrame : public GSFrame, public GSFrameBase
{
	wxGLCanvas* canvas;
	u32 m_frames;

	GLGSFrame();

	void close() override;

	bool shown() override;
	void hide() override;
	void show() override;

	void* make_context() override;
	void set_current(draw_context_t context) override;
	void delete_context(void* context) override;
	void flip(draw_context_t context) override;
	size2i client_size() override;

	wxGLCanvas* GetCanvas() const { return canvas; }

	virtual void SetViewport(int x, int y, u32 w, u32 h) override;

private:
	virtual void OnSize(wxSizeEvent& event);
};