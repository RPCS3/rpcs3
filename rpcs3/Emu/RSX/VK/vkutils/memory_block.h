#pragma once

#include "../VulkanAPI.h"
#include "mem_allocator.h"

namespace vk
{
	struct memory_block
	{
		memory_block(VkDevice dev, u64 block_sz, u64 alignment, u32 memory_type_index);
		~memory_block();

		VkDeviceMemory get_vk_device_memory();
		u64 get_vk_device_memory_offset();

		void* map(u64 offset, u64 size);
		void unmap();

		memory_block(const memory_block&) = delete;
		memory_block(memory_block&&)      = delete;

	private:
		VkDevice m_device;
		vk::mem_allocator_base* m_mem_allocator;
		mem_allocator_base::mem_handle_t m_mem_handle;
	};
}
