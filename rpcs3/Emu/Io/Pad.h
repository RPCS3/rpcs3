#pragma once

#include "PadHandler.h"

class PadManager //: public wxWindow
{
	bool m_inited;
	PadHandlerBase* m_pad_handler;

public:
	PadManager();
	~PadManager();

	void Init(const u32 max_connect);
	void Close();

	Array<Pad>& GetPads() { return m_pad_handler->GetPads(); }
	PadInfo& GetInfo() { return m_pad_handler->GetInfo(); }
	Array<Button>& GetButtons(const u32 pad) { return m_pad_handler->GetButtons(pad); }

	bool IsInited() const { return m_inited; }

//private:
	//DECLARE_EVENT_TABLE();
};