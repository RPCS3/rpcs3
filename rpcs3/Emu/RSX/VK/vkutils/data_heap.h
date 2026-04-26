#pragma once

#include "../../Common/ring_buffer_helper.h"
#include "../VulkanAPI.h"
#include "buffer_object.h"
#include "commands.h"
#include "ex.h"

#include <memory>
#include <type_traits>
#include <vector>

namespace vk
{
	enum data_heap_pool_flags
	{
		heap_pool_default            = 0,
		heap_pool_low_latency        = (1 << 0),
		heap_pool_fixed_size         = (1 << 1),
		heap_pool_force_vram_shadow  = (1 << 2),
	};

	class data_heap : public ::data_heap
	{
	private:
		usz initial_size = 0;
		bool mapped = false;
		void* _ptr = nullptr;

		rsx::flags32_t m_flags = heap_pool_default;

		bool notify_on_grow = false;
		bool m_prefer_writethrough = false;

		std::unique_ptr<buffer> shadow;
		std::vector<VkBufferCopy> dirty_ranges;

		mutable utils::address_range64 m_cached_buffer_range{};

	protected:
		bool grow(usz size) override;
		bool can_allocate_heap(const vk::memory_type_info& target_heap, usz size, int max_usage_percent);

	public:
		std::unique_ptr<buffer> heap;

		// NOTE: Some drivers (RADV) use heavyweight OS map/unmap routines that are insanely slow
		// Avoid mapping/unmapping to keep these drivers from stalling
		// NOTE2: HOST_CACHED flag does not keep the mapped ptr around in the driver either

		void create(VkBufferUsageFlags usage, usz size, rsx::flags32_t flags, const char* name, usz guard = 0x10000, VkBool32 notify = VK_FALSE);
		void destroy();

		void* map(usz offset, usz size);
		void unmap(bool force = false);

		template<int Alignment, typename T = char>
			requires std::is_trivially_destructible_v<T>
		std::pair<usz, T*> alloc_and_map(usz count)
		{
			const auto size_bytes = count * sizeof(T);
			const auto addr = alloc<Alignment>(size_bytes);
			return { addr, reinterpret_cast<T*>(map(addr, size_bytes)) };
		}

		void sync(const vk::command_buffer& cmd);

		template <usz Alignment>
		VkDescriptorBufferInfoEx window(usz offset, usz range, u64 window_size) const
		{
			if (window_size > size())
			{
				// Driver specific. AMD allows viewing upto 4GB as UBO no problem.
				return { *heap, 0, VK_WHOLE_SIZE };
			}

			if (utils::address_range64::start_length(offset, range).inside(m_cached_buffer_range)) [[ likely ]]
			{
				return { *heap, m_cached_buffer_range.start, m_cached_buffer_range.length() };
			}

			const u64 aligned_window_size = static_cast<u64>(window_size / Alignment) * Alignment;
			const u64 start_partition = offset / aligned_window_size;
			const u64 end_partition = (offset + range - 1) / aligned_window_size;

			if (start_partition == end_partition) [[ likely ]]
			{
				m_cached_buffer_range = utils::address_range64::start_length(start_partition * aligned_window_size, aligned_window_size);
				return { *heap, m_cached_buffer_range.start, m_cached_buffer_range.length() };
			}

			// We have a partition straddler. Invalidate caching and return exact range.
			m_cached_buffer_range = {};
			return { *heap, offset, range };
		}

		// Properties
		bool is_dirty() const;
		bool has_shadow() const { return !!shadow; }
	};

	extern data_heap* get_upload_heap();
}
