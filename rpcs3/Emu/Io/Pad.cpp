#include "stdafx.h"
#include "Emu/ConLog.h"
#include "rpcs3/Ini.h"
#include "Pad.h"
#include "Null/NullPadHandler.h"

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
	int numHandlers = rPlatform::getPadHandlerCount();
	int selectedHandler = Ini.PadHandlerMode.GetValue();
	if (selectedHandler > numHandlers)
	{
		selectedHandler = 0;
	}
	m_pad_handler.reset(rPlatform::getPadHandler(selectedHandler));

	m_pad_handler->Init(max_connect);
	m_inited = true;
}

void PadManager::Close()
{
	if(m_pad_handler) m_pad_handler->Close();
	m_pad_handler = nullptr;

	m_inited = false;
}