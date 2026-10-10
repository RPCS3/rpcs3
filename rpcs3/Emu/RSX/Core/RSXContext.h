#pragma once

#include <util/types.hpp>
#include "Emu/RSX/NV47/FW/context.h"
#include "Emu/RSX/NV47/HW/context.h"

namespace rsx
{
	class thread;
	struct rsx_state;

	// Master RSX context. Aggregates the domain-specific sub-objects from HW and FW context information.
	struct context
	{
		thread* rsxthr = nullptr;
		rsx_state* register_state = nullptr;

		context(u32 id, thread* owner);
		~context();

		context(const context&) = delete;
		context& operator=(const context&) = delete;

		// Stable identity. 0 is reserved for the primary context.
		u32 id() const { return m_id; }

	private:
		u32 m_id = 0;
	};
}
