#include "stdafx.h"
#include "VKResourceManager.h"
#include "VKGSRender.h"
#include "VKCommandStream.h"

namespace vk
{
	std::unordered_map<uptr, vmm_allocation_t> g_vmm_allocations;
	std::unordered_map<uptr, atomic_t<u64>> g_vmm_memory_usage;

	resource_manager g_resource_manager;
	atomic_t<u64> g_event_ctr;
	atomic_t<u64> g_last_completed_event;

	constexpr u64 s_vmm_warn_threshold_size = 2000 * 0x100000; // Warn if allocation on a single heap exceeds this value

	resource_manager* get_resource_manager()
	{
		return &g_resource_manager;
	}

	u64 get_event_id()
	{
		return g_event_ctr++;
	}

	u64 current_event_id()
	{
		return g_event_ctr.load();
	}

	u64 last_completed_event_id()
	{
		return g_last_completed_event.load();
	}

	void on_event_completed(u64 event_id, bool flush)
	{
		if (!flush && g_cfg.video.multithreaded_rsx)
		{
			auto& offloader_thread = g_fxo->get<rsx::dma_manager>();
			ensure(!offloader_thread.is_current_thread());

			offloader_thread.backend_ctrl(rctrl_run_gc, reinterpret_cast<void*>(event_id));
			return;
		}

		g_resource_manager.eid_completed(event_id);
		g_last_completed_event = std::max(event_id, g_last_completed_event.load());
	}

	static constexpr f32 size_in_GiB(u64 size)
	{
		return size / (1024.f * 1024.f * 1024.f);
	}

	void vmm_notify_memory_allocated(void* handle, u32 memory_type, u64 memory_size)
	{
		auto key = reinterpret_cast<uptr>(handle);
		const vmm_allocation_t info = { memory_size, memory_type };

		if (const auto ins = g_vmm_allocations.insert_or_assign(key, info);
			!ins.second)
		{
			rsx_log.error("Duplicate vmm entry with memory handle 0x%llx", key);
		}

		auto& vmm_size = g_vmm_memory_usage[memory_type];
		vmm_size += memory_size;

		if (vmm_size > s_vmm_warn_threshold_size && (vmm_size - memory_size) <= s_vmm_warn_threshold_size)
		{
			rsx_log.warning("Memory type 0x%x has allocated more than %.2fG. Currently allocated %.2fG",
				memory_type, size_in_GiB(s_vmm_warn_threshold_size), size_in_GiB(vmm_size));
		}
	}

	void vmm_notify_memory_freed(void* handle)
	{
		auto key = reinterpret_cast<uptr>(handle);
		if (auto found = g_vmm_allocations.find(key);
			found != g_vmm_allocations.end())
		{
			const auto& info = found->second;
			g_vmm_memory_usage[info.type_index] -= info.size;
			g_vmm_allocations.erase(found);
		}
	}

	void vmm_reset()
	{
		g_vmm_memory_usage.clear();
		g_vmm_allocations.clear();
	}

	bool vmm_handle_memory_pressure(rsx::problem_severity severity)
	{
		if (auto vkthr = dynamic_cast<VKGSRender*>(rsx::get_current_renderer()))
		{
			return vkthr->on_vram_exhausted(severity);
		}

		return false;
	}

	void vmm_check_memory_usage()
	{
		const auto vmm_load = get_current_mem_allocator()->get_memory_usage();
		rsx::problem_severity load_severity = rsx::problem_severity::low;

		if (vmm_load > 90.f)
		{
			rsx_log.warning("Video memory usage exceeding 90%. Will attempt to reclaim resources.");
			load_severity = rsx::problem_severity::severe;
		}
		else if (vmm_load > 75.f)
		{
			rsx_log.notice("Video memory usage exceeding 75%. Will attempt to reclaim resources.");
			load_severity = rsx::problem_severity::moderate;
		}

		if (load_severity >= rsx::problem_severity::moderate)
		{
			vmm_handle_memory_pressure(load_severity);
		}
	}
}
