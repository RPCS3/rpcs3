#pragma once

#include <util/types.hpp>
#include "context.h"
#include "context_accessors.define.h"

namespace rsx
{
	enum command_barrier_type : u32;
	enum vertex_base_type;

	namespace util
	{
		u32 get_report_data_impl(rsx::context* ctx, u32 offset);

		void push_vertex_data(rsx::context* ctx, u32 attrib_index, u32 channel_select, int count, rsx::vertex_base_type vtype, u32 value);

		void push_draw_parameter_change(rsx::context* ctx, rsx::command_barrier_type type, u32 reg, u32 arg);

		template <bool FlushDMA, bool FlushPipe>
		void write_gcm_label(context* ctx, u32 address, u32 data)
		{
			const bool is_flip_sema = (address == (RSX(ctx)->label_addr + 0x10) || address == (RSX(ctx)->device_addr + 0x30));
			if (!is_flip_sema)
			{
				// First, queue the GPU work. If it flushes the queue for us, the following routines will be faster.
				const bool handled = RSX(ctx)->get_backend_config().supports_host_gpu_labels && RSX(ctx)->release_GCM_label(address, data);

				if (vm::_ref<RsxSemaphore>(address).val == data)
				{
					// It's a no-op to write the same value (although there is a delay in real-hw so it's more accurate to allow GPU label in this case)
					return;
				}

				if constexpr (FlushDMA)
				{
					// If the backend handled the request, this call will basically be a NOP
					g_fxo->get<rsx::dma_manager>().sync();
				}

				if constexpr (FlushPipe)
				{
					// Manually flush the pipeline.
					// It is possible to stream report writes using the host GPU, but that generates too much submit traffic.
					RSX(ctx)->sync();
				}

				if (handled)
				{
					// Backend will handle it, nothing to write.
					return;
				}
			}

			vm::_ref<RsxSemaphore>(address).val = data;
		}
	}
}
 
#include "context_accessors.undef.h"
