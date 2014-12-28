#pragma once

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
	SPURSManager();

	void Finalize();
	void AttachLv2EventQueue(u32 queue, vm::ptr<u8> port, int isDynamic);
	void DetachLv2EventQueue(u8 port);
};
