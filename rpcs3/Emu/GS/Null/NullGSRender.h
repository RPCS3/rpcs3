#pragma once
#include "Emu/GS/GSRender.h"

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

	virtual void ExecCMD()
	{
	}

	virtual void Flip()
	{
	}

	virtual void Close()
	{
	}
};
