#pragma once
#include "Emu/RSX/GSRender.h"

class NullGSRender final : public GSRender
{
public:

	NullGSRender()
	{
	}

	virtual ~NullGSRender() override
	{
	}

private:
	virtual void OnInit() override
	{
	}

	virtual void OnInitThread() override
	{
	}

	virtual void OnExitThread() override
	{
	}

	virtual void OnReset() override
	{
	}

	virtual void Clear(u32 cmd) override
	{
	}

	virtual void Draw() override
	{
	}

	virtual void Flip() override
	{
	}

	virtual void Close() override
	{
		if (joinable())
		{
			join();
		}
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
