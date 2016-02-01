#pragma once

#include "Emu/Io/KeyboardHandler.h"

class BasicKeyboardHandler final : public KeyboardHandlerBase, public wxWindow
{
public:
	virtual void Init(const u32 max_connect) override;

	BasicKeyboardHandler();

	void KeyDown(wxKeyEvent& event);
	void KeyUp(wxKeyEvent& event);
	void LoadSettings();
};
