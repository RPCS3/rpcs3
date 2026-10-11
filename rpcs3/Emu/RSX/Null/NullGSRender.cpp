#include "stdafx.h"
#include "NullGSRender.h"

u64 NullGSRender::get_cycles()
{
	return thread_ctrl::get_cycles(static_cast<named_thread<NullGSRender>&>(*this));
}

NullGSRender::NullGSRender(utils::serial* ar) noexcept : GSRender(ar)
{
}
