#include "stdafx.h"
#include "Emu/RSX/rsx_methods.h"
#include "RSXContext.h"

namespace rsx
{
	context::context(u32 id, thread* owner)
		: rsxthr(owner)
		, m_id(id)
		, m_owned_register_state(std::make_unique<rsx_state>())
	{
		register_state = m_owned_register_state.get();
		register_state->init();
	}

	context::~context() = default;
}
