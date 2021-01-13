#include "device.h"
#include "memory.h"

namespace vk
{
	mem_allocator_vma::mem_allocator_vma(VkDevice dev, VkPhysicalDevice pdev) : mem_allocator_base(dev, pdev)
	{
		// Initialize stats pool
		std::fill(stats.begin(), stats.end(), VmaBudget{});

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = pdev;
		allocatorInfo.device = dev;

		vmaCreateAllocator(&allocatorInfo, &m_allocator);
	}

	void mem_allocator_vma::destroy()
	{
		vmaDestroyAllocator(m_allocator);
	}

	mem_allocator_vk::mem_handle_t mem_allocator_vma::alloc(u64 block_sz, u64 alignment, u32 memory_type_index)
	{
		VmaAllocation vma_alloc;
		VkMemoryRequirements mem_req = {};
		VmaAllocationCreateInfo create_info = {};

		mem_req.memoryTypeBits = 1u << memory_type_index;
		mem_req.size = block_sz;
		mem_req.alignment = alignment;
		create_info.memoryTypeBits = 1u << memory_type_index;

		if (VkResult result = vmaAllocateMemory(m_allocator, &mem_req, &create_info, &vma_alloc, nullptr);
			result != VK_SUCCESS)
		{
			if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY &&
				vmm_handle_memory_pressure(rsx::problem_severity::fatal))
			{
				// If we just ran out of VRAM, attempt to release resources and try again
				result = vmaAllocateMemory(m_allocator, &mem_req, &create_info, &vma_alloc, nullptr);
			}

			if (result != VK_SUCCESS)
			{
				die_with_error(result);
			}
			else
			{
				rsx_log.warning("Renderer ran out of video memory but successfully recovered.");
			}
		}

		vmm_notify_memory_allocated(vma_alloc, memory_type_index, block_sz);
		return vma_alloc;
	}

	void mem_allocator_vma::free(mem_handle_t mem_handle)
	{
		vmm_notify_memory_freed(mem_handle);
		vmaFreeMemory(m_allocator, static_cast<VmaAllocation>(mem_handle));
	}

	void* mem_allocator_vma::map(mem_handle_t mem_handle, u64 offset, u64 /*size*/)
	{
		void* data = nullptr;

		CHECK_RESULT(vmaMapMemory(m_allocator, static_cast<VmaAllocation>(mem_handle), &data));

		// Add offset
		data = static_cast<u8*>(data) + offset;
		return data;
	}

	void mem_allocator_vma::unmap(mem_handle_t mem_handle)
	{
		vmaUnmapMemory(m_allocator, static_cast<VmaAllocation>(mem_handle));
	}

	VkDeviceMemory mem_allocator_vma::get_vk_device_memory(mem_handle_t mem_handle)
	{
		VmaAllocationInfo alloc_info;

		vmaGetAllocationInfo(m_allocator, static_cast<VmaAllocation>(mem_handle), &alloc_info);
		return alloc_info.deviceMemory;
	}

	u64 mem_allocator_vma::get_vk_device_memory_offset(mem_handle_t mem_handle)
	{
		VmaAllocationInfo alloc_info;

		vmaGetAllocationInfo(m_allocator, static_cast<VmaAllocation>(mem_handle), &alloc_info);
		return alloc_info.offset;
	}

	f32 mem_allocator_vma::get_memory_usage()
	{
		vmaGetBudget(m_allocator, stats.data());

		float max_usage = 0.f;
		for (const auto& info : stats)
		{
			if (!info.budget)
			{
				break;
			}

			const float this_usage = (info.usage * 100.f) / info.budget;
			max_usage = std::max(max_usage, this_usage);
		}

		return max_usage;
	}

	mem_allocator_vk::mem_handle_t mem_allocator_vk::alloc(u64 block_sz, u64 /*alignment*/, u32 memory_type_index)
	{
		VkDeviceMemory memory;
		VkMemoryAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		info.allocationSize = block_sz;
		info.memoryTypeIndex = memory_type_index;

		if (VkResult result = vkAllocateMemory(m_device, &info, nullptr, &memory); result != VK_SUCCESS)
		{
			if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY && vmm_handle_memory_pressure(rsx::problem_severity::fatal))
			{
				// If we just ran out of VRAM, attempt to release resources and try again
				result = vkAllocateMemory(m_device, &info, nullptr, &memory);
			}

			if (result != VK_SUCCESS)
			{
				die_with_error(result);
			}
			else
			{
				rsx_log.warning("Renderer ran out of video memory but successfully recovered.");
			}
		}

		vmm_notify_memory_allocated(memory, memory_type_index, block_sz);
		return memory;
	}

	void mem_allocator_vk::free(mem_handle_t mem_handle)
	{
		vmm_notify_memory_freed(mem_handle);
		vkFreeMemory(m_device, static_cast<VkDeviceMemory>(mem_handle), nullptr);
	}

	void* mem_allocator_vk::map(mem_handle_t mem_handle, u64 offset, u64 size)
	{
		void* data = nullptr;
		CHECK_RESULT(vkMapMemory(m_device, static_cast<VkDeviceMemory>(mem_handle), offset, std::max<u64>(size, 1u), 0, &data));
		return data;
	}

	void mem_allocator_vk::unmap(mem_handle_t mem_handle)
	{
		vkUnmapMemory(m_device, static_cast<VkDeviceMemory>(mem_handle));
	}

	VkDeviceMemory mem_allocator_vk::get_vk_device_memory(mem_handle_t mem_handle)
	{
		return static_cast<VkDeviceMemory>(mem_handle);
	}

	u64 mem_allocator_vk::get_vk_device_memory_offset(mem_handle_t /*mem_handle*/)
	{
		return 0;
	}

	f32 mem_allocator_vk::get_memory_usage()
	{
		return 0.f;
	}

	mem_allocator_base* get_current_mem_allocator()
	{
		return g_render_device->get_allocator();
	}

	memory_block::memory_block(VkDevice dev, u64 block_sz, u64 alignment, u32 memory_type_index)
		: m_device(dev)
	{
		m_mem_allocator = get_current_mem_allocator();
		m_mem_handle    = m_mem_allocator->alloc(block_sz, alignment, memory_type_index);
	}

	memory_block::~memory_block()
	{
		m_mem_allocator->free(m_mem_handle);
	}

	VkDeviceMemory memory_block::get_vk_device_memory()
	{
		return m_mem_allocator->get_vk_device_memory(m_mem_handle);
	}

	u64 memory_block::get_vk_device_memory_offset()
	{
		return m_mem_allocator->get_vk_device_memory_offset(m_mem_handle);
	}

	void* memory_block::map(u64 offset, u64 size)
	{
		return m_mem_allocator->map(m_mem_handle, offset, size);
	}

	void memory_block::unmap()
	{
		m_mem_allocator->unmap(m_mem_handle);
	}
}
