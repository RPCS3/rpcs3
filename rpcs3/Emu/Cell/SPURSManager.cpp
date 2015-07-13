#include "stdafx.h"
#include "Emu/Memory/Memory.h"

#include "SPURSManager.h"

SPURSManagerEventFlag::SPURSManagerEventFlag(u32 flagClearMode, u32 flagDirection)
{
	this->flagClearMode = flagClearMode;
	this->flagDirection = flagDirection;
}

SPURSManagerTasksetAttribute::SPURSManagerTasksetAttribute(u64 args, vm::ptr<const u8> priority, u32 maxContention)
{
	this->args = args;
	this->maxContention = maxContention;
}

SPURSManager::SPURSManager()
{
}

void SPURSManager::Finalize()
{
}

void SPURSManager::AttachLv2EventQueue(u32 queue, vm::ptr<u8> port, int isDynamic)
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