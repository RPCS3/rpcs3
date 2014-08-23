#pragma once
#include "Emu/RSX/RSXThread.h"

struct GSRender : public RSXThread
{
	virtual ~GSRender()
	{
	}

	virtual void Close()=0;
};

enum GSLockType
{
	GS_LOCK_NOT_WAIT,
	GS_LOCK_WAIT_FLUSH,
	GS_LOCK_WAIT_FLIP,
};

struct GSLock
{
private:
	GSRender& m_renderer;
	GSLockType m_type;

public:
	GSLock(GSRender& renderer, GSLockType type);

	~GSLock();
};

struct GSLockCurrent : GSLock
{
	GSLockCurrent(GSLockType type);
};