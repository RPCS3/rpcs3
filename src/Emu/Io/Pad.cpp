#include "stdafx.h"
#include "Emu/System.h"

#include "Pad.h"

void PadManager::Init(u32 max_connect)
{
	m_pad_handler = Emu.GetCallbacks().get_pad_handler();
	m_pad_handler->Init(max_connect);
}

void PadManager::Close()
{
	m_pad_handler.reset();
}
