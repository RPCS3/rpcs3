#pragma once

#include "Emu/SysCalls/Modules/cellSpurs.h"

// Internal class to shape a SPURS attribute.
class SPURSManagerAttribute
{
public:
	SPURSManagerAttribute(int nSpus, int spuPriority, int ppuPriority, bool exitIfNoWork);

	int _setNamePrefix(const char *name, u32 size);

	int _setSpuThreadGroupType(int type);

	int _setMemoryContainerForSpuThread(u32 container);

protected:
	be_t<int> nSpus; 
	be_t<int> spuThreadGroupPriority; 
	be_t<int> ppuThreadPriority; 
	bool exitIfNoWork;
	char namePrefix[CELL_SPURS_NAME_MAX_LENGTH+1];
	be_t<int> threadGroupType;
	be_t<u32> container;
};

class SPURSManagerEventFlag
{
public:
	SPURSManagerEventFlag(u32 flagClearMode, u32 flagDirection);

	u32 _getDirection() const
	{
		return this->flagDirection;
	}

	u32 _getClearMode() const
	{
		return this->flagClearMode;
	}

protected:
	be_t<u32> flagClearMode;
	be_t<u32> flagDirection;
};

class SPURSManagerTasksetAttribute
{
public:
	SPURSManagerTasksetAttribute(u64 args, vm::ptr<const u8> priority, u32 maxContention);

protected:
	be_t<u64> args;
	be_t<u32> maxContention;
};

class SPURSManagerTaskset
{
public:
	SPURSManagerTaskset(u32 address, SPURSManagerTasksetAttribute *tattr);

protected:
	u32 address;
	SPURSManagerTasksetAttribute *tattr;
};

// Main SPURS manager class.
class SPURSManager
{
public:
	SPURSManager(SPURSManagerAttribute *attr);

	void Finalize();
	void AttachLv2EventQueue(u32 queue, vm::ptr<u8> port, int isDynamic);
	void DetachLv2EventQueue(u8 port);

protected:
	SPURSManagerAttribute *attr;
};
