#pragma once

#include <util/types.hpp>
#include "Emu/RSX/RSXThread.h"

#include "context_accessors.define.h"

namespace rsx
{
	namespace util
	{
		template <bool FlushDMA, bool FlushPipe>
		static void write_gcm_label(context* ctx, u32 address, u32 data)
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
