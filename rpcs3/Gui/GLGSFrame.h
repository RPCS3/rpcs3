#pragma once
#include "wx/glcanvas.h"
#include "Gui/GSFrame.h"

struct GLGSFrame : public GSFrame
{
	wxGLCanvas* canvas;
	u32 m_frames;

	GLGSFrame();
	~GLGSFrame() {}

	void Flip(wxGLContext *context);

	wxGLCanvas* GetCanvas() const { return canvas; }

	virtual void SetViewport(int x, int y, u32 w, u32 h);

private:
	virtual void OnSize(wxSizeEvent& event);
};