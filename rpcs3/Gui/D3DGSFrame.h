#pragma once
#include "Emu/RSX/D3D12/D3D12GSRender.h"
#include "Gui/GSFrame.h"
#include "wx/window.h"

struct D3DGSFrame : public GSFrame, public GSFrameBase2
{
	wxWindow* canvas;
	u32 m_frames;

	D3DGSFrame();
	~D3DGSFrame();

	virtual void Close() override;

	virtual bool IsShown() override;
	virtual void Hide() override;
	virtual void Show() override;

	virtual void* GetNewContext() override;
	virtual void SetCurrent(void* ctx) override;
	virtual void DeleteContext(void* ctx) override;
	virtual void Flip(void* context) override;

	wxWindow* GetCanvas() const { return canvas; }

	virtual void SetViewport(int x, int y, u32 w, u32 h) override;
	virtual HWND getHandle() const override;

private:
	virtual void OnSize(wxSizeEvent& event);
};