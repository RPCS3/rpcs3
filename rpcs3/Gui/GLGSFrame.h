#pragma once
#include "wx/glcanvas.h"
#include "Gui/GSFrame.h"
#include "Emu/RSX/GL/GLGSRender.h"

struct GLGSFrame : public GSFrame, public GSFrameBase
{
	wxGLCanvas* canvas;
	u32 m_frames;

	GLGSFrame();
	~GLGSFrame();

	virtual void Close() override;

	virtual bool IsShown() override;
	virtual void Hide() override;
	virtual void Show() override;

	virtual void* GetNewContext() override;
	virtual void SetCurrent(void* ctx) override;
	virtual void DeleteContext(void* ctx) override;
	virtual void Flip(void* context) override;

	wxGLCanvas* GetCanvas() const { return canvas; }

	virtual void SetViewport(int x, int y, u32 w, u32 h);

private:
	virtual void OnSize(wxSizeEvent& event);
};