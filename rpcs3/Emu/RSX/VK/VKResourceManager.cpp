#include "stdafx.h"
#include "VKResourceManager.h"

namespace vk
{
	std::unordered_map<uintptr_t, vmm_allocation_t> g_vmm_allocations;
	std::unordered_map<uintptr_t, atomic_t<u64>> g_vmm_memory_usage;

	resource_manager g_resource_manager;
	atomic_t<u64> g_event_ctr;

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

	void on_event_completed(u64 event_id)
	{
		// TODO: Offload this to a secondary thread
		g_resource_manager.eid_completed(event_id);
	}

	static constexpr f32 size_in_GiB(u64 size)
	{
		return size / (1024.f * 1024.f * 1024.f);
	}

	void vmm_notify_memory_allocated(void* handle, u32 memory_type, u64 memory_size)
	{
		auto key = reinterpret_cast<uintptr_t>(handle);
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
		auto key = reinterpret_cast<uintptr_t>(handle);
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
}
