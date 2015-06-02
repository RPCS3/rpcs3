#pragma once
#include "Emu/RSX/GSRender.h"

class NullGSRender : public GSRender
{
public:

	NullGSRender()
	{
	}

	virtual ~NullGSRender()
	{
	}

private:
	void oninit() override
	{
	}

	void oninit_thread() override
	{
	}
	
	void onexit_thread() override
	{
	}

	void onreset() override
	{
	}

	bool domethod(u32 cmd, u32 value) override
	{
		return false;
	}

	void flip(int buffer) override
	{
	}

	virtual void Close()
	{
	}
};