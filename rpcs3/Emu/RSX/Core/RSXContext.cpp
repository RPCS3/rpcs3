#include "stdafx.h"
#include "Emu/RSX/rsx_methods.h"
#include "RSXContext.h"

namespace rsx
{
	context::context(u32 id, thread* owner)
		: rsxthr(owner)
		, register_state(&method_registers)
		, m_id(id)
	{
	}

	context::~context() = default;
}
