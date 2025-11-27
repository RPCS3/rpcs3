#include "barriers.h"
#include "data_heap.h"
#include "device.h"

#include "../../RSXOffload.h"
#include "../VKHelpers.h"
#include "../VKResourceManager.h"
#include "Emu/IdManager.h"

#include <memory>

namespace vk
{
	data_heap g_upload_heap;

	void data_heap::create(VkBufferUsageFlags usage, usz size, rsx::flags32_t flags, const char* name, usz guard, VkBool32 notify)
	{
		::data_heap::init(size, name, guard);

		const auto& memory_map = g_render_device->get_memory_mapping();

		if ((flags & heap_pool_low_latency) && g_cfg.video.vk.use_rebar_upload_heap)
		{
			// Prefer uploading to BAR if low latency is desired.
			const int max_usage = memory_map.device_bar_total_bytes <= (256 * 0x100000) ? 75 : 90;
			m_prefer_writethrough = can_allocate_heap(memory_map.device_bar, size, max_usage);

			// Log it
			if (m_prefer_writethrough)
			{
				rsx_log.notice("Data heap %s will attempt to use Re-BAR memory", m_name);
			}
			else
			{
				rsx_log.warning("Could not fit heap '%s' into Re-BAR memory", m_name);
			}
		}

		VkFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		auto memory_index = m_prefer_writethrough ? memory_map.device_bar : memory_map.host_visible_coherent;

		if (!(get_heap_compatible_buffer_types() & usage))
		{
			rsx_log.warning("Buffer usage %u is not heap-compatible using this driver, explicit staging buffer in use", usage);

			shadow = std::make_unique<buffer>(*g_render_device, size, memory_index, memory_flags, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 0, VMM_ALLOCATION_POOL_SYSTEM);
			usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			memory_index = memory_map.device_local;
			m_prefer_writethrough = false;
		}

		VkFlags create_flags = 0;
		if (m_prefer_writethrough)
		{
			create_flags |= VK_BUFFER_CREATE_ALLOW_NULL_RPCS3;
		}

		heap = std::make_unique<buffer>(*g_render_device, size, memory_index, memory_flags, usage, create_flags, VMM_ALLOCATION_POOL_SYSTEM);

		if (!heap->value)
		{
			rsx_log.warning("Could not place heap '%s' into Re-BAR memory. Will attempt to use regular host-visible memory.", m_name);
			ensure(m_prefer_writethrough);

			// We failed to place the buffer in rebar memory. Try again in host-visible.
			m_prefer_writethrough = false;
			auto gc = get_resource_manager();
			gc->dispose(heap);
			heap = std::make_unique<buffer>(*g_render_device, size, memory_map.host_visible_coherent, memory_flags, usage, 0, VMM_ALLOCATION_POOL_SYSTEM);
		}

		initial_size = size;
		notify_on_grow = bool(notify);
	}

	void data_heap::destroy()
	{
		if (mapped)
		{
			unmap(true);
		}

		heap.reset();
		shadow.reset();
	}

	bool data_heap::grow(usz size)
	{
		// Create new heap. All sizes are aligned up by 64M, upto 1GiB
		const usz size_limit = 1024 * 0x100000;
		usz aligned_new_size = utils::align(m_size + size, 64 * 0x100000);

		if (aligned_new_size >= size_limit)
		{
			// Too large, try to swap out the heap instead of growing.
			rsx_log.error("[%s] Pool limit was reached. Will attempt to swap out the current heap.", m_name);
			aligned_new_size = size_limit;
		}

		// Wait for DMA activity to end
		g_fxo->get<rsx::dma_manager>().sync();

		if (mapped)
		{
			// Force reset mapping
			unmap(true);
		}

		VkBufferUsageFlags usage = heap->info.usage;
		const auto& memory_map = g_render_device->get_memory_mapping();

		if (m_prefer_writethrough)
		{
			const int max_usage = memory_map.device_bar_total_bytes <= (256 * 0x100000) ? 75 : 90;
			m_prefer_writethrough = can_allocate_heap(memory_map.device_bar, aligned_new_size, max_usage);

			if (!m_prefer_writethrough)
			{
				rsx_log.warning("Could not fit heap '%s' into Re-BAR memory during reallocation", m_name);
			}
		}

		VkFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		auto memory_index = m_prefer_writethrough ? memory_map.device_bar : memory_map.host_visible_coherent;

		// Update heap information and reset the allocator
		::data_heap::init(aligned_new_size, m_name, m_min_guard_size);

		// Discard old heap and create a new one. Old heap will be garbage collected when no longer needed
		auto gc = get_resource_manager();
		if (shadow)
		{
			ensure(!m_prefer_writethrough);
			rsx_log.warning("Buffer usage %u is not heap-compatible using this driver, explicit staging buffer in use", usage);

			gc->dispose(shadow);
			shadow = std::make_unique<buffer>(*g_render_device, aligned_new_size, memory_index, memory_flags, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 0, VMM_ALLOCATION_POOL_SYSTEM);
			usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			memory_index = memory_map.device_local;
		}

		gc->dispose(heap);

		VkFlags create_flags = 0;
		if (m_prefer_writethrough)
		{
			create_flags |= VK_BUFFER_CREATE_ALLOW_NULL_RPCS3;
		}

		heap = std::make_unique<buffer>(*g_render_device, aligned_new_size, memory_index, memory_flags, usage, create_flags, VMM_ALLOCATION_POOL_SYSTEM);

		if (!heap->value)
		{
			rsx_log.warning("Could not place heap '%s' into Re-BAR memory. Will attempt to use regular host-visible memory.", m_name);
			ensure(m_prefer_writethrough);

			// We failed to place the buffer in rebar memory. Try again in host-visible.
			m_prefer_writethrough = false;
			gc->dispose(heap);
			heap = std::make_unique<buffer>(*g_render_device, aligned_new_size, memory_map.host_visible_coherent, memory_flags, usage, 0, VMM_ALLOCATION_POOL_SYSTEM);
		}

		if (notify_on_grow)
		{
			raise_status_interrupt(vk::heap_changed);
		}

		return true;
	}

	bool data_heap::can_allocate_heap(const vk::memory_type_info& target_heap, usz size, int max_usage_percent)
	{
		const auto current_usage = vmm_get_application_memory_usage(target_heap);
		const auto after_usage = current_usage + size;
		const auto limit = (target_heap.total_bytes() * max_usage_percent) / 100;
		return after_usage < limit;
	}

	void* data_heap::map(usz offset, usz size)
	{
		if (!_ptr)
		{
			if (shadow)
				_ptr = shadow->map(0, shadow->size());
			else
				_ptr = heap->map(0, heap->size());

			mapped = true;
		}

		if (shadow)
		{
			dirty_ranges.push_back({ offset, offset, size });
			raise_status_interrupt(runtime_state::heap_dirty);
		}

		return static_cast<u8*>(_ptr) + offset;
	}

	void data_heap::unmap(bool force)
	{
		if (force)
		{
			if (shadow)
				shadow->unmap();
			else
				heap->unmap();

			mapped = false;
			_ptr = nullptr;
		}
	}

	void data_heap::sync(const vk::command_buffer& cmd)
	{
		if (!dirty_ranges.empty())
		{
			ensure(shadow);
			ensure(heap);
			vkCmdCopyBuffer(cmd, shadow->value, heap->value, ::size32(dirty_ranges), dirty_ranges.data());
			dirty_ranges.clear();

			insert_buffer_memory_barrier(cmd, heap->value, 0, heap->size(),
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
		}
	}

	bool data_heap::is_dirty() const
	{
		return !dirty_ranges.empty();
	}

	data_heap* get_upload_heap()
	{
		if (!g_upload_heap.heap)
		{
			g_upload_heap.create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 64 * 0x100000, vk::heap_pool_default, "auxilliary upload heap", 0x100000);
		}

		return &g_upload_heap;
	}
}
