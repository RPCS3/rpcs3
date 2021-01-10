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

	void data_heap::create(VkBufferUsageFlags usage, usz size, const char* name, usz guard, VkBool32 notify)
	{
		::data_heap::init(size, name, guard);

		const auto& memory_map = g_render_device->get_memory_mapping();

		VkFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		auto memory_index = memory_map.host_visible_coherent;

		if (!(get_heap_compatible_buffer_types() & usage))
		{
			rsx_log.warning("Buffer usage %u is not heap-compatible using this driver, explicit staging buffer in use", usage);

			shadow = std::make_unique<buffer>(*g_render_device, size, memory_index, memory_flags, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 0);
			usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			memory_index = memory_map.device_local;
		}

		heap = std::make_unique<buffer>(*g_render_device, size, memory_index, memory_flags, usage, 0);

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
		const usz aligned_new_size = utils::align(m_size + size, 64 * 0x100000);

		if (aligned_new_size >= size_limit)
		{
			// Too large
			return false;
		}

		if (shadow)
		{
			// Shadowed. Growing this can be messy as it requires double allocation (macOS only)
			return false;
		}

		// Wait for DMA activity to end
		g_fxo->get<rsx::dma_manager>()->sync();

		if (mapped)
		{
			// Force reset mapping
			unmap(true);
		}

		VkBufferUsageFlags usage = heap->info.usage;
		const auto& memory_map = g_render_device->get_memory_mapping();

		VkFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		auto memory_index = memory_map.host_visible_coherent;

		// Update heap information and reset the allocator
		::data_heap::init(aligned_new_size, m_name, m_min_guard_size);

		// Discard old heap and create a new one. Old heap will be garbage collected when no longer needed
		get_resource_manager()->dispose(heap);
		heap = std::make_unique<buffer>(*g_render_device, aligned_new_size, memory_index, memory_flags, usage, 0);

		if (notify_on_grow)
		{
			raise_status_interrupt(vk::heap_changed);
		}

		return true;
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

	bool data_heap::is_critical() const
	{
		if (!::data_heap::is_critical())
			return false;

		// By default, allow the size to grow upto 8x larger
		// This value is arbitrary, theoretically it is possible to allow infinite stretching to improve performance
		const usz soft_limit = initial_size * 8;
		if ((m_size + m_min_guard_size) < soft_limit)
			return false;

		return true;
	}

	data_heap* get_upload_heap()
	{
		if (!g_upload_heap.heap)
		{
			g_upload_heap.create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 64 * 0x100000, "auxilliary upload heap", 0x100000);
		}

		return &g_upload_heap;
	}
}
