#pragma once
#include "Emu/RSX/GL/GLGSRender.h"
#include "Gui/GSFrame.h"
#include "wx/glcanvas.h"

struct D3D9GSFrame : public GSFrame, public GSFrameBase
{
	u32 m_frames;

	D3D9GSFrame();
	~D3D9GSFrame();

	virtual void Close() override;

	virtual bool IsShown() override;
	virtual void Hide() override;
	virtual void Show() override;

	virtual void* GetNewContext() override;
	virtual void SetCurrent(void* ctx) override;
	virtual void DeleteContext(void* ctx) override;
	virtual void Flip(void* context) override;
	sizei GetClientSize() override;

	virtual void SetViewport(int x, int y, u32 w, u32 h);

private:
	virtual void OnSize(wxSizeEvent& event);
};