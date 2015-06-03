#pragma once
#include "Emu/RSX/RSXThread.h"

class GSFrameBase
{
public:
	GSFrameBase() {}
	GSFrameBase(const GSFrameBase&) = delete;
	virtual void Close() = 0;

	virtual bool IsShown() = 0;
	virtual void Hide() = 0;
	virtual void Show() = 0;

	virtual void* GetNewContext() = 0;
	virtual void SetCurrent(void* ctx) = 0;
	virtual void DeleteContext(void* ctx) = 0;
	virtual void Flip(void* ctx) = 0;
	virtual sizei GetClientSize() = 0;
};

enum class GSFrameType
{
	OpenGLFrame
};

using GetGSFrameCb = GSFrameBase* (*)(GSFrameType);
extern GetGSFrameCb GetGSFrame;
void SetGetGSFrameCallback(GetGSFrameCb value);

struct GSRender : public rsx::thread
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