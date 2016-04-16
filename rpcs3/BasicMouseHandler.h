#pragma once

#include "Emu/Io/MouseHandler.h"

class BasicMouseHandler final : public MouseHandlerBase, public wxWindow
{
public:
	virtual void Init(const u32 max_connect) override;

	BasicMouseHandler();

	void MouseButtonDown(wxMouseEvent& event);
	void MouseButtonUp(wxMouseEvent& event);
	void MouseScroll(wxMouseEvent& event);
	void MouseMove(wxMouseEvent& event);
};
