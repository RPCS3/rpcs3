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
	void OnInit() override
	{
	}

	void OnInitThread() override
	{
	}

	void OnExitThread() override
	{
	}

	void OnReset() override
	{
	}

	void ExecCMD(u32 cmd) override
	{
	}

	void ExecCMD() override
	{
	}

	void Flip(int buffer) override
	{
	}

	void Close() override
	{
	}
};