#include "stdafx.h"
#include "NullGSRender.h"
#include "Emu/System.h"

NullGSRender::NullGSRender() : GSRender(frame_type::Null)
{
}

bool NullGSRender::do_method(u32 cmd, u32 value)
{
	return false;
}
