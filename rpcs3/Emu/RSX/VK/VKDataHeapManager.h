#pragma once

#include <util/types.hpp>

#include <unordered_map>

namespace vk
{
	class data_heap;

	namespace data_heap_manager
	{
		using managed_heap_snapshot_t = std::unordered_map<const vk::data_heap*, s64>;

		// Submit ring buffer for management
		void register_ring_buffer(vk::data_heap& heap);

		// Bulk registration
		void register_ring_buffers(std::initializer_list<std::reference_wrapper<vk::data_heap>> heaps);

		// Capture managed ring buffers snapshot at current time
		managed_heap_snapshot_t get_heap_snapshot();

		// Synchronize heap with snapshot
		void restore_snapshot(const managed_heap_snapshot_t& snapshot);

		// Reset all managed heap allocations
		void reset_heap_allocations();

		// Cleanup
		void reset();
	}
}
