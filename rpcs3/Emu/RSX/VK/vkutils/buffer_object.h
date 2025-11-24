#pragma once

#include "../VulkanAPI.h"
#include "device.h"
#include "memory.h"
#include "unique_resource.h"

namespace vk
{
	enum : u32
	{
		VK_BUFFER_CREATE_ALLOW_NULL_RPCS3 = 0x80000000,

		VK_BUFFER_CREATE_SPECIAL_FLAGS_RPCS3 = (VK_BUFFER_CREATE_ALLOW_NULL_RPCS3)
	};

	struct buffer_view : public unique_resource
	{
		VkBufferView value;
		VkBufferViewCreateInfo info = {};

		buffer_view(VkDevice dev, VkBuffer buffer, VkFormat format, VkDeviceSize offset, VkDeviceSize size);
		~buffer_view();

		buffer_view(const buffer_view&) = delete;
		buffer_view(buffer_view&&)      = delete;

		bool in_range(u32 address, u32 size, u32& offset) const;

	private:
		VkDevice m_device;
	};

	struct buffer : public unique_resource
	{
		VkBuffer value;
		VkBufferCreateInfo info = {};
		std::unique_ptr<vk::memory_block> memory;

		buffer(
			const vk::render_device& dev,
			u64 size,
			const memory_type_info& memory_type,
			u32 access_flags,
			VkBufferUsageFlags usage,
			VkBufferCreateFlags flags,
			vmm_allocation_pool allocation_pool);

		buffer(
			const vk::render_device& dev,
			VkBufferUsageFlags usage,
			void* host_pointer,
			u64 size);

		~buffer();

		void* map(u64 offset, u64 size);
		void unmap();
		u32 size() const;

		buffer(const buffer&) = delete;
		buffer(buffer&&) = delete;

	private:
		VkDevice m_device;
	};
}
