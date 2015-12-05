#include "stdafx.h"
#include "NullGSRender.h"
#include "Emu/System.h"

NullGSRender::NullGSRender() : GSRender(frame_type::Null)
{
}

void NullGSRender::onexit_thread()
{
}

bool NullGSRender::domethod(u32 cmd, u32 value)
{
	return false;
}
