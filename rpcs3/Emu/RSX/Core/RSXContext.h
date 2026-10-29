#pragma once

#include <util/types.hpp>
#include <memory>
#include "Emu/RSX/Common/bitfield.hpp"
#include "Emu/RSX/NV47/FW/context.h"
#include "Emu/RSX/NV47/HW/context.h"
#include "Emu/RSX/RSXFIFO.h"
#include "Emu/RSX/RSXZCULL.h"
#include "RSXContextResources.h"
#include "RSXCoreRenderer.h"
#include "RSXDisplay.h"
#include "RSXDrawCommands.h"
#include "RSXDrawState.h"
#include "RSXDriverState.h"
#include "RSXFrameBuffer.h"
#include "RSXProgramState.h"
#include "RSXTextureState.h"

namespace rsx
{
	class thread;
	struct rsx_state;

	// Master RSX context. Aggregates the domain-specific sub-objects from HW and FW context information.
	struct context
	{
		thread* rsxthr = nullptr;
		rsx_state* register_state = nullptr;
		core_renderer core{};

		// lv2 driver state of this context
		fw_context fw;

		// Draw sequencing and the immediate-mode push buffers of this context
		draw_command_processor draw_processor;

		// Virtual FIFO channel. Saved here while the context is not active.
		FIFO::channel_state fifo{};

		// Pipeline state
		bitmask_t<pipeline_state> graphics_state;
		bool in_begin_end = false;

		// Decoded pipeline state for the backend
		draw_state_t draw_state;

		framebuffer_state fb;
		program_state program;
		texture_state textures;
		reports::conditional_render_eval cond_render;

		frame_statistics_t stats{};
		frame_skip_state frame_skip;

		// Last draw timestamp as a monotonic tag. Mostly used to invalidate caches.
		u64 ROP_sync_timestamp = 0;

		// Display buffers registered by the lv2 driver. Per-context.
		RsxDisplayInfo display_buffers[8];
		u32 display_buffers_count{ 0 };
		u32 current_display_buffer{ 0 };

		// Host resources of the render backend. Core manages lifetime.
		std::unique_ptr<context_resources> resources;

		context(u32 id, thread* owner);
		~context();

		context(const context&) = delete;
		context& operator=(const context&) = delete;

		// Stable identity. 0 is reserved for the primary context.
		u32 id() const { return m_id; }

		// Use to invalidate all state decoder caching flags. This is what makes a context switch expensive.
		void mark_all_dirty();

	private:
		u32 m_id = 0;
		std::unique_ptr<rsx_state> m_owned_register_state;
	};
}
