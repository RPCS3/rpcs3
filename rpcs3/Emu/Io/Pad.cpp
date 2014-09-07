#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Null/NullPadHandler.h"
#include "Pad.h"

GetPadHandlerCountCb GetPadHandlerCount = []()
{
	return 1;
};

GetPadHandlerCb GetPadHandler = [](int i) -> PadHandlerBase*
{
	return new NullPadHandler;
};

void SetGetPadHandlerCountCallback(GetPadHandlerCountCb cb)
{
	GetPadHandlerCount = cb;
}

void SetGetPadHandlerCallback(GetPadHandlerCb cb)
{
	GetPadHandler = cb;
}

PadManager::PadManager()
	: m_pad_handler(nullptr)
	, m_inited(false)
{
}

PadManager::~PadManager()
{
}

void PadManager::Init(const u32 max_connect)
{
	if(m_inited)
		return;

	// NOTE: Change these to std::make_unique assignments when C++14 is available.
	int numHandlers = GetPadHandlerCount();
	int selectedHandler = Ini.PadHandlerMode.GetValue();
	if (selectedHandler > numHandlers)
	{
		selectedHandler = 0;
	}
	m_pad_handler.reset(GetPadHandler(selectedHandler));

	m_pad_handler->Init(max_connect);
	m_inited = true;
}

void PadManager::Close()
{
	if(m_pad_handler) m_pad_handler->Close();
	m_pad_handler = nullptr;

	m_inited = false;
}