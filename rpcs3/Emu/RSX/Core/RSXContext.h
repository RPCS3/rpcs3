#pragma once

#include <util/types.hpp>
#include <memory>
#include "Emu/RSX/Common/bitfield.hpp"
#include "Emu/RSX/NV47/FW/context.h"
#include "Emu/RSX/NV47/HW/context.h"
#include "RSXDriverState.h"
#include "RSXFrameBuffer.h"

namespace rsx
{
	class thread;
	struct rsx_state;

	// Master RSX context. Aggregates the domain-specific sub-objects from HW and FW context information.
	struct context
	{
		thread* rsxthr = nullptr;
		rsx_state* register_state = nullptr;

		// Pipeline state
		bitmask_t<pipeline_state> graphics_state;
		atomic_bitmask_t<eng_interrupt_reason> eng_interrupt_mask;
		bool in_begin_end = false;

		framebuffer_state fb;

		context(u32 id, thread* owner);
		~context();

		context(const context&) = delete;
		context& operator=(const context&) = delete;

		// Stable identity. 0 is reserved for the primary context.
		u32 id() const { return m_id; }

	private:
		u32 m_id = 0;
		std::unique_ptr<rsx_state> m_owned_register_state;
	};
}
