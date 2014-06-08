#include "stdafx.h"
#include "SPURSManager.h"
#include "Emu/Memory/Memory.h"

SPURSManager::SPURSManager(SPURSManagerAttribute *attr)
{
	this->attr = attr;
}

void SPURSManager::Finalize()
{
	delete this->attr;
}

void SPURSManager::AttachLv2EventQueue(u32 queue, mem8_t port, int isDynamic)
{
	//TODO:
}

void SPURSManager::DetachLv2EventQueue(u8 port)
{
	//TODO:
}

SPURSManagerTaskset::SPURSManagerTaskset(u32 address, SPURSManagerTasksetAttribute *tattr)
{
	this->tattr = tattr;
	this->address = address;
}
