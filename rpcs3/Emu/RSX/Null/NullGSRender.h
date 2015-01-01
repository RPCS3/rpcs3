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

	virtual void Enable(u32 cmd, u32 enable)
	{
	}

	virtual void ClearColor(u32 a, u32 r, u32 g, u32 b)
	{
	}

	virtual void ClearStencil(u32 stencil)
	{
	}

	virtual void ClearDepth(u32 depth)
	{
	}

	virtual void ClearSurface(u32 mask)
	{
	}

	virtual void ColorMask(bool a, bool r, bool g, bool b)
	{
	}

	virtual void ExecCMD()
	{
	}

	virtual void AlphaFunc(u32 func, float ref)
	{
	}

	virtual void DepthFunc(u32 func)
	{
	}

	virtual void DepthMask(u32 flag)
	{
	}

	virtual void Flip()
	{
	}

	virtual void Close()
	{
	}
};
