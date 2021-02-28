#include "buffer_object.h"
#include "device.h"
#include "shared.h"

namespace vk
{
	buffer_view::buffer_view(VkDevice dev, VkBuffer buffer, VkFormat format, VkDeviceSize offset, VkDeviceSize size)
	    : m_device(dev)
	{
		info.buffer = buffer;
		info.format = format;
		info.offset = offset;
		info.range  = size;
		info.sType  = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
		CHECK_RESULT(vkCreateBufferView(m_device, &info, nullptr, &value));
	}

	buffer_view::~buffer_view()
	{
		vkDestroyBufferView(m_device, value, nullptr);
	}

	bool buffer_view::in_range(u32 address, u32 size, u32& offset) const
	{
		if (address < info.offset)
			return false;

		const u32 _offset = address - static_cast<u32>(info.offset);
		if (info.range < _offset)
			return false;

		const auto remaining = info.range - _offset;
		if (size <= remaining)
		{
			offset = _offset;
			return true;
		}

		return false;
	}

		buffer::buffer(const vk::render_device& dev, u64 size, u32 memory_type_index, u32 access_flags, VkBufferUsageFlags usage, VkBufferCreateFlags flags)
			: m_device(dev)
		{
			info.size = size;
			info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			info.flags = flags;
			info.usage = usage;

			CHECK_RESULT(vkCreateBuffer(m_device, &info, nullptr, &value));

			//Allocate vram for this buffer
			VkMemoryRequirements memory_reqs;
			vkGetBufferMemoryRequirements(m_device, value, &memory_reqs);

			if (!(memory_reqs.memoryTypeBits & (1 << memory_type_index)))
			{
				//Suggested memory type is incompatible with this memory type.
				//Go through the bitset and test for requested props.
				if (!dev.get_compatible_memory_type(memory_reqs.memoryTypeBits, access_flags, &memory_type_index))
					fmt::throw_exception("No compatible memory type was found!");
			}

			memory = std::make_unique<memory_block>(m_device, memory_reqs.size, memory_reqs.alignment, memory_type_index);
			vkBindBufferMemory(dev, value, memory->get_vk_device_memory(), memory->get_vk_device_memory_offset());
		}

		buffer::buffer(const vk::render_device& dev, VkBufferUsageFlags usage, void* host_pointer, u64 size)
			: m_device(dev)
		{
			info.size = size;
			info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			info.flags = 0;
			info.usage = usage;

			VkExternalMemoryBufferCreateInfoKHR ex_info;
			ex_info.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO_KHR;
			ex_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
			ex_info.pNext = nullptr;

			info.pNext = &ex_info;
			CHECK_RESULT(vkCreateBuffer(m_device, &info, nullptr, &value));

			auto& memory_map = dev.get_memory_mapping();
			u32 memory_type_index = memory_map.host_visible_coherent;
			VkFlags access_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

			ensure(memory_map._vkGetMemoryHostPointerPropertiesEXT);

			VkMemoryHostPointerPropertiesEXT memory_properties{};
			memory_properties.sType = VK_STRUCTURE_TYPE_MEMORY_HOST_POINTER_PROPERTIES_EXT;
			CHECK_RESULT(memory_map._vkGetMemoryHostPointerPropertiesEXT(dev, VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT, host_pointer, &memory_properties));

			VkMemoryRequirements memory_reqs;
			vkGetBufferMemoryRequirements(m_device, value, &memory_reqs);

			auto required_memory_type_bits = memory_reqs.memoryTypeBits & memory_properties.memoryTypeBits;
			if (!required_memory_type_bits)
			{
				// AMD driver bug. Buffers created with external memory extension return type bits of 0
				rsx_log.warning("Could not match buffer requirements and host pointer properties.");
				required_memory_type_bits = memory_properties.memoryTypeBits;
			}

			if (!dev.get_compatible_memory_type(required_memory_type_bits, access_flags, &memory_type_index))
			{
				fmt::throw_exception("No compatible memory type was found!");
			}

			memory = std::make_unique<memory_block_host>(m_device, host_pointer, size, memory_type_index);
			CHECK_RESULT(vkBindBufferMemory(dev, value, memory->get_vk_device_memory(), memory->get_vk_device_memory_offset()));
		}

		buffer::~buffer()
		{
			vkDestroyBuffer(m_device, value, nullptr);
		}

		void* buffer::map(u64 offset, u64 size)
		{
			return memory->map(offset, size);
		}

		void buffer::unmap()
		{
			memory->unmap();
		}

		u32 buffer::size() const
		{
			return static_cast<u32>(info.size);
		}
}
