#pragma once

#include "Gui/GSFrame.h"
#include "wx/glcanvas.h"

class GLGSFrame : public GSFrame
{
	wxGLCanvas* m_canvas;

public:
	GLGSFrame(size2i);

	void* make_context() override;
	void set_current(draw_context_t context) override;
	void delete_context(void* context) override;
	void flip(draw_context_t context) override;

private:
	virtual void OnSize(wxSizeEvent& event);
};
