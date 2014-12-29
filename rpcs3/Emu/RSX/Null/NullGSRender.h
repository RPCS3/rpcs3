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

	virtual void ClearColor()
	{
	}

	virtual void ClearStencil()
	{
	}

	virtual void ClearDepth()
	{
	}

	virtual void ClearSurface()
	{
	}

	virtual void ColorMask()
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
