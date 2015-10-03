#pragma once
#include "Emu/RSX/GSRender.h"

class NullGSRender final : public GSRender
{
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

	bool domethod(u32 cmd, u32 value) override
	{
		return false;
	}

	void flip(int buffer) override
	{
	}

	void close() override
	{
	}
};
