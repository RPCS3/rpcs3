#pragma once
#include "Emu/RSX/GSRender.h"

class NullGSRender
	: public GSRender
{
public:

	NullGSRender()
	{
	}

	virtual ~NullGSRender()
	{
	}

private:
	virtual void OnInit()
	{
	}

	virtual void OnInitThread()
	{
	}

	virtual void OnExitThread()
	{
	}

	virtual void OnReset()
	{
	}

	virtual void Clear(u32 cmd) override
	{
	}

	virtual void Draw() override
	{
	}

	virtual void Flip()
	{
	}

	virtual void Close()
	{
	}

	virtual void semaphorePGRAPHTextureReadRelease(u32 offset, u32 value) override
	{
	}

	virtual void semaphorePGRAPHBackendRelease(u32 offset, u32 value) override
	{
	}

	virtual void semaphorePFIFOAcquire(u32 offset, u32 value) override
	{
	}
};