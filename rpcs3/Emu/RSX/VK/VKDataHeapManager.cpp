#include "stdafx.h"
#include "VKDataHeapManager.h"

#include "vkutils/data_heap.h"
#include <unordered_set>

namespace vk::data_heap_manager
{
	std::unordered_set<vk::data_heap*> g_managed_heaps;

	void register_ring_buffer(vk::data_heap& heap)
	{
		g_managed_heaps.insert(&heap);
	}

	void register_ring_buffers(std::initializer_list<std::reference_wrapper<vk::data_heap>> heaps)
	{
		for (auto&& heap : heaps)
		{
			register_ring_buffer(heap);
		}
	}

	managed_heap_snapshot_t get_heap_snapshot()
	{
		managed_heap_snapshot_t result{};
		for (auto& heap : g_managed_heaps)
		{
			result[heap] = heap->get_current_put_pos_minus_one();
		}
		return result;
	}

	void restore_snapshot(const managed_heap_snapshot_t& snapshot)
	{
		for (auto& heap : g_managed_heaps)
		{
			const auto found = snapshot.find(heap);
			if (found == snapshot.end())
			{
				continue;
			}

			heap->set_get_pos(found->second);
			heap->notify();
		}
	}

	void reset_heap_allocations()
	{
		for (auto& heap : g_managed_heaps)
		{
			heap->reset_allocation_stats();
		}
	}

	void reset()
	{
		for (auto& heap : g_managed_heaps)
		{
			heap->destroy();
		}

		g_managed_heaps.clear();
	}
}
