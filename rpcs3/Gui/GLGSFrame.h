#pragma once
#include "Emu/RSX/GL/GLGSRender.h"
#include "Gui/GSFrame.h"
#include "wx/glcanvas.h"

class GLGSFrame : public GSFrame
{
	wxGLCanvas* m_canvas;
	u32 m_frames;
public:
	GLGSFrame();
	~GLGSFrame();

	virtual void* make_context() override;
	virtual void set_current(draw_context_t context) override;
	virtual void delete_context(void* ctx) override;
	virtual void flip(draw_context_t context) override;

private:
	virtual void OnSize(wxSizeEvent& event);
};