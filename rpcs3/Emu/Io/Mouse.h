#pragma once
#include "MouseHandler.h"

class MouseManager
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
	MouseData& GetData(const u32 mouse) { return m_mouse_handler->GetData(mouse); }
	MouseRawData& GetRawData(const u32 mouse) { return m_mouse_handler->GetRawData(mouse); }

	bool IsInited() const { return m_inited; }
};

typedef int(*GetMouseHandlerCountCb)();
typedef MouseHandlerBase*(*GetMouseHandlerCb)(int i);

void SetGetMouseHandlerCountCallback(GetMouseHandlerCountCb cb);
void SetGetMouseHandlerCallback(GetMouseHandlerCb cb);