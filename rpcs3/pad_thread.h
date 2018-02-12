#pragma once

#include <map>
#include <thread>

#include "../Utilities/types.h"
#include "Emu/Io/PadHandler.h"

struct PadInfo
{
	u32 max_connect;
	u32 now_connect;
	u32 system_info;
};

class pad_thread
{
public:
	pad_thread(void *_curthread, void *_curwindow); //void * instead of QThread * and QWindow * because of include in emucore
	~pad_thread();

	void Init(const u32 max_connect);
	PadInfo& GetInfo() { return m_info; }
	std::vector<std::shared_ptr<Pad>>& GetPads() { return m_pads; }
	void SetRumble(const u32 pad, u8 largeMotor, bool smallMotor);

protected:
	void ThreadFunc();

	//List of all handlers
	std::map<pad_handler, std::shared_ptr<PadHandlerBase>> handlers;

	//Used for pad_handler::keyboard
	void *curthread;
	void *curwindow;

	PadInfo m_info;
	std::vector<std::shared_ptr<Pad>> m_pads;

	bool active;
	std::shared_ptr<std::thread> thread;
};
