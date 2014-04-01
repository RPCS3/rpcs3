#include "stdafx.h"
#include "Pad.h"
#include "Null/NullPadHandler.h"
#include "Windows/WindowsPadHandler.h"

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
	switch(Ini.PadHandlerMode.GetValue())
	{
	case 1:
		m_pad_handler.reset(new WindowsPadHandler());
		break;

	default:
	case 0:
		m_pad_handler.reset(new NullPadHandler());
		break;
	}

	m_pad_handler->Init(max_connect);
	m_inited = true;
}

void PadManager::Close()
{
	if(m_pad_handler) m_pad_handler->Close();
	m_pad_handler = nullptr;

	m_inited = false;
}