#pragma once
#include "Emu/RSX/GSRender.h"

class NullGSRender final : public GSRender
{
public:

	NullGSRender() : GSRender(frame_type::Null)
	{
	}

	virtual ~NullGSRender() override
	{
	}

private:
	virtual void oninit() override
	{
	}

	virtual void oninit_thread() override
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

	virtual void semaphorePGRAPHTextureReadRelease(u32 offset, u32 value) override
	{
	}

	virtual void semaphorePGRAPHBackendRelease(u32 offset, u32 value) override
	{
	}

	virtual void semaphorePFIFOAcquire(u32 offset, u32 value) override
	{
	}

	virtual void notifyProgramChange() override {}
	virtual void notifyBlendStateChange() override {}
	virtual void notifyDepthStencilStateChange() override {}
	virtual void notifyRasterizerStateChange() override {}
};
