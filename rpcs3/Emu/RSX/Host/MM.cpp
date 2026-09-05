#include "stdafx.h"
#include "MM.h"
#include <Emu/RSX/Common/simple_array.hpp>
#include <Emu/RSX/RSXOffload.h>
#include <Emu/RSX/rsx_utils.h>

#include <Emu/Memory/vm.h>
#include <Emu/IdManager.h>
#include <Emu/system_config.h>
#include <Utilities/address_range.h>
#include <Utilities/mutex.h>

namespace rsx
{
	rsx::simple_array<MM_block> g_deferred_mprotect_queue;
	shared_mutex g_mprotect_queue_lock;

	void mm_sanitize_queue_internal()
	{
		u32 w = 0, r = 0;
		for (; r < g_deferred_mprotect_queue.size(); ++r)
		{
			auto& block = g_deferred_mprotect_queue[r];
			if (!block.range.valid())
			{
				continue;
			}
			g_deferred_mprotect_queue[w++] = block;
		}
		g_deferred_mprotect_queue.resize(w);
	}

	void mm_flush_mprotect_queue_internal(u32 count)
	{
		AUDIT(count <= g_deferred_mprotect_queue.size());

		for (u32 i = 0; i < count; ++i)
		{
			auto& block = g_deferred_mprotect_queue[i];
			ensure(block.range.valid());

			utils::memory_protect(reinterpret_cast<void*>(block.range.start), block.range.length(), block.prot);
			block.range.invalidate();
		}

		const u32 remaining = g_deferred_mprotect_queue.size() - count;
		if (!remaining)
		{
			g_deferred_mprotect_queue.clear();
			return;
		}

		// Pop processed entries from the queue
		mm_sanitize_queue_internal();
	}

	// Reverse scan to find the latest overlapping MM conflict. The result is a count of prefix blocks.
	template <typename F>
	u32 mm_find_conflict_internal(F&& predicate)
	{
		for (u32 i = g_deferred_mprotect_queue.size(); i > 0; --i)
		{
			if (std::invoke(predicate, g_deferred_mprotect_queue[i - 1]))
			{
				return i;
			}
		}

		return 0;
	}

	void mm_defer_mprotect_internal(u64 start, u64 length, utils::protection prot)
	{
		const auto range = utils::address_range64::start_length(start, length);
		bool has_invalid = false;
		bool is_merged = false;

		// Attempt a merge first. The queue length is short but the time taken to run mprotect is very high in comparison.
		for (auto it = g_deferred_mprotect_queue.rbegin();
			it != g_deferred_mprotect_queue.rend();
			++it)
		{
			auto& block = *it;
			if (!block.touches(range))
			{
				continue;
			}

			if (block.prot != prot)
			{
				// Optimization. If our new range swallows the old one, replace it.
				if (block.range.inside(range))
				{
					block.range.invalidate();
					has_invalid = true;
					continue;
				}

				if (!block.overlaps(range))
				{
					// Adjacent. Skip.
					continue;
				}

				// Preserve ordering. Do not proceed with merge.
				break;
			}

			block.merge(range);
			is_merged = true;
			break;
		}

		if (has_invalid)
		{
			mm_sanitize_queue_internal();
		}

		if (is_merged)
		{
			return;
		}

		g_deferred_mprotect_queue.push_back({ range, prot, rsx::get_shared_tag() });
	}

	void mm_protect(void* ptr, u64 length, utils::protection prot)
	{
		if (g_cfg.video.disable_async_host_memory_manager)
		{
			utils::memory_protect(ptr, length, prot);
			return;
		}

		// Naive merge. Eventually it makes more sense to do conflict resolution, but it's not as important.
		const auto start = reinterpret_cast<u64>(ptr);
		const auto range = utils::address_range64::start_length(start, length);

		std::lock_guard lock(g_mprotect_queue_lock);

		if (prot == utils::protection::rw || prot == utils::protection::wx)
		{
			// Basically an unlock op. Flush the conflicting prefix block if any overlap is detected.
			if (u32 count = mm_find_conflict_internal(FN(x.overlaps(range))))
			{
				// Check for degenerate ranges that we'll be crushing
				for (auto pblock = &g_deferred_mprotect_queue[count - 1];
					count > 0 && pblock->range.inside(range);
					count--, pblock--)
				{
					pblock->range.invalidate();
				}

				if (count)
				{
					mm_flush_mprotect_queue_internal(count);
				}
				else
				{
					mm_sanitize_queue_internal();
				}
			}

			utils::memory_protect(ptr, length, prot);
			return;
		}

		// No, Ro, etc.
		mm_defer_mprotect_internal(start, length, prot);
	}

	void mm_flush()
	{
		std::lock_guard lock(g_mprotect_queue_lock);
		mm_flush_mprotect_queue_internal(g_deferred_mprotect_queue.size());
	}

	void mm_flush(u32 vm_address)
	{
		std::lock_guard lock(g_mprotect_queue_lock);
		if (g_deferred_mprotect_queue.empty())
		{
			return;
		}

		const auto addr = reinterpret_cast<u64>(vm::base(vm_address));
		if (const u32 count = mm_find_conflict_internal(FN(x.overlaps(addr))))
		{
			mm_flush_mprotect_queue_internal(count);
		}
	}

	void mm_flush(const rsx::simple_array<utils::address_range64>& ranges)
	{
		std::lock_guard lock(g_mprotect_queue_lock);
		if (g_deferred_mprotect_queue.empty() || ranges.empty())
		{
			return;
		}

		const auto block_overlaps_ranges = [&](const MM_block& block) { return ranges.any(FN(block.overlaps(x))); };
		if (const u32 count = mm_find_conflict_internal(block_overlaps_ranges))
		{
			mm_flush_mprotect_queue_internal(count);
		}
	}

	void mm_flush_lazy()
	{
		if (!g_cfg.video.multithreaded_rsx)
		{
			mm_flush();
			return;
		}

		std::lock_guard lock(g_mprotect_queue_lock);
		if (g_deferred_mprotect_queue.empty())
		{
			return;
		}

		auto& rsxdma = g_fxo->get<rsx::dma_manager>();
		rsxdma.backend_ctrl(mm_backend_ctrl::cmd_mm_flush, nullptr);
	}

	void mm_flush_partial(u64 last_tag)
	{
		std::lock_guard lock(g_mprotect_queue_lock);

		u32 count = 0;
		for (const auto& block : g_deferred_mprotect_queue)
		{
			if (block.sync_tag > last_tag)
			{
				break;
			}
			count++;
		}

		if (count)
		{
			mm_flush_mprotect_queue_internal(count);
		}
	}
}
