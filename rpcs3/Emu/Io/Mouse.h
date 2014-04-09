#pragma once

#include <memory>
#include <vector>
#include "MouseHandler.h"

class MouseManager //: public wxWindow
{
	bool m_inited;
	std::unique_ptr<MouseHandlerBase> m_mouse_handler;

public:
	MouseManager();
	~MouseManager();

	void Init(const u32 max_connect);
	void Close();

	std::vector<Mouse>& GetMice() { return m_mouse_handler->GetMice(); }
	MouseInfo& GetInfo() { return m_mouse_handler->GetInfo(); }
	CellMouseData& GetData(const u32 mouse) { return m_mouse_handler->GetData(mouse); }
	CellMouseRawData& GetRawData(const u32 mouse) { return m_mouse_handler->GetRawData(mouse); }

	bool IsInited() const { return m_inited; }

//private:
	//DECLARE_EVENT_TABLE();
};