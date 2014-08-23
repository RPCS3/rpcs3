#include "stdafx.h"
#include "Emu/Memory/Memory.h"

#include "SPURSManager.h"

SPURSManagerAttribute::SPURSManagerAttribute(int nSpus, int spuPriority, int ppuPriority, bool exitIfNoWork)
{
	this->nSpus = nSpus;
	this->spuThreadGroupPriority = spuPriority;
	this->ppuThreadPriority = ppuPriority;
	this->exitIfNoWork = exitIfNoWork;

	memset(this->namePrefix, 0, CELL_SPURS_NAME_MAX_LENGTH + 1);
	this->threadGroupType = 0;
	this->container = 0;
}

int SPURSManagerAttribute::_setNamePrefix(const char *name, u32 size)
{
	strncpy(this->namePrefix, name, size);
	this->namePrefix[0] = 0;
	return 0;
}

int SPURSManagerAttribute::_setSpuThreadGroupType(int type)
{
	this->threadGroupType = type;
	return 0;
}

int SPURSManagerAttribute::_setMemoryContainerForSpuThread(u32 container)
{
	this->container = container;
	return 0;
}

SPURSManagerEventFlag::SPURSManagerEventFlag(u32 flagClearMode, u32 flagDirection)
{
	this->flagClearMode = flagClearMode;
	this->flagDirection = flagDirection;
}

SPURSManagerTasksetAttribute::SPURSManagerTasksetAttribute(u64 args, mem8_t priority, u32 maxContention)
{
	this->args = args;
	this->maxContention = maxContention;
}

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