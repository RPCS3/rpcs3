#include "stdafx.h"
#include "NullGSRender.h"
#include "Emu/System.h"

u64 NullGSRender::get_cycles()
{
	return thread_ctrl::get_cycles(static_cast<named_thread<NullGSRender>&>(*this));
}

NullGSRender::NullGSRender() : GSRender()
{
}

bool NullGSRender::do_method(u32 cmd, u32 value)
{
	return false;
}

void NullGSRender::end()
{
	rsx::method_registers.current_draw_clause.end();
}
