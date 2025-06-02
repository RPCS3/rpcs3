#include "device.h"
#include "memory.h"

namespace
{
	// Copied from rsx_utils.h. Move to a more convenient location
	template<typename T, typename U>
	static inline T align2(T value, U alignment)
	{
		return ((value + alignment - 1) / alignment) * alignment;
	}
}

namespace vk
{
	memory_type_info::memory_type_info(u32 index, u64 size)
	{
		push(index, size);
	}
	void memory_type_info::push(u32 index, u64 size)
	{
		type_ids.push_back(index);
		type_sizes.push_back(size);
	}

	memory_type_info::const_iterator memory_type_info::begin() const
	{
		return type_ids.data();
	}

	memory_type_info::const_iterator memory_type_info::end() const
	{
		return type_ids.data() + type_ids.size();
	}

	u32 memory_type_info::first() const
	{
		ensure(!type_ids.empty());
		return type_ids.front();
	}

	size_t memory_type_info::count() const
	{
		return type_ids.size();
	}

	memory_type_info::operator bool() const
	{
		return !type_ids.empty();
	}

	bool memory_type_info::operator == (const memory_type_info& other) const
	{
		if (type_ids.size() != other.type_ids.size())
		{
			return false;
		}

		switch (type_ids.size())
		{
		case 1:
			return type_ids[0] == other.type_ids[0];
		case 2:
			return ((type_ids[0] == other.type_ids[0] && type_ids[1] == other.type_ids[1]) ||
					(type_ids[0] == other.type_ids[1] && type_ids[1] == other.type_ids[0]));
		default:
			for (const auto& id : other.type_ids)
			{
				if (std::find(type_ids.begin(), type_ids.end(), id) == type_ids.end())
				{
					return false;
				}
			}
			return true;
		}
	}

	memory_type_info memory_type_info::get(const render_device& dev, u32 access_flags, u32 type_mask) const
	{
		memory_type_info result{};
		for (size_t i = 0; i < type_ids.size(); ++i)
		{
			if (type_mask & (1 << type_ids[i]))
			{
				result.push(type_ids[i], type_sizes[i]);
			}
		}

		if (!result)
		{
			u32 type;
			if (dev.get_compatible_memory_type(type_mask, access_flags, &type))
			{
				result = { type, 0ull };
			}
		}

		return result;
	}

	void memory_type_info::rebalance()
	{
		// Re-order indices with the least used one first.
		// This will avoid constant pressure on the memory budget in low memory systems.

		if (type_ids.size() <= 1)
		{
			// Nothing to do
			return;
		}

		std::vector<std::pair<u32, u64>> free_memory_map;
		const auto num_types = type_ids.size();
		u64 last_free = UINT64_MAX;
		bool to_reorder = false;

		for (u32 i = 0; i < num_types; ++i)
		{
			const auto heap_size = type_sizes[i];
			const auto type_id = type_ids[i];
			ensure(heap_size > 0);

			const u64 used_mem = vmm_get_application_memory_usage({ type_id, 0ull });
			const u64 free_mem = (used_mem >= heap_size) ? 0ull : (heap_size - used_mem);

			to_reorder |= (free_mem > last_free);
			last_free = free_mem;

			free_memory_map.push_back({ i, free_mem });
		}

		if (!to_reorder) [[likely]]
		{
			return;
		}

		ensure(free_memory_map.size() == num_types);
		std::sort(free_memory_map.begin(), free_memory_map.end(), FN(x.second > y.second));

		std::vector<u32> new_type_ids(num_types);
		std::vector<u64> new_type_sizes(num_types);

		for (u32 i = 0; i < num_types; ++i)
		{
			const u32 ref = free_memory_map[i].first;
			new_type_ids[i] = type_ids[ref];
			new_type_sizes[i] = type_sizes[ref];
		}

		type_ids = new_type_ids;
		type_sizes = new_type_sizes;

		rsx_log.warning("Rebalanced memory types successfully");
	}

	mem_allocator_base::mem_allocator_base(const vk::render_device& dev, VkPhysicalDevice /*pdev*/)
		: m_device(dev), m_allocation_flags(0)
	{}

	mem_allocator_vma::mem_allocator_vma(const vk::render_device& dev, VkPhysicalDevice pdev, VkInstance inst)
		: mem_allocator_base(dev, pdev)
	{
		// Initialize stats pool
		std::fill(stats.begin(), stats.end(), VmaBudget{});

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = pdev;
		allocatorInfo.device = dev;
		allocatorInfo.instance = inst;
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;

		std::vector<VkDeviceSize> heap_limits;
		const auto vram_allocation_limit = g_cfg.video.vk.vram_allocation_limit * 0x100000ull;
		if (vram_allocation_limit < dev.get_memory_mapping().device_local_total_bytes)
		{
			VkPhysicalDeviceMemoryProperties memory_properties;
			vkGetPhysicalDeviceMemoryProperties(pdev, &memory_properties);
			for (u32 i = 0; i < memory_properties.memoryHeapCount; ++i)
			{
				const u64 max_sz = (memory_properties.memoryHeaps[i].flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
					? vram_allocation_limit
					: VK_WHOLE_SIZE;

				heap_limits.push_back(max_sz);
			}
			allocatorInfo.pHeapSizeLimit = heap_limits.data();
		}

		CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &m_allocator));

		// Allow fastest possible allocation on start
		set_fastest_allocation_flags();
	}

	void mem_allocator_vma::destroy()
	{
		vmaDestroyAllocator(m_allocator);
	}

	mem_allocator_vk::mem_handle_t mem_allocator_vma::alloc(u64 block_sz, u64 alignment, const memory_type_info& memory_type, vmm_allocation_pool pool, bool throw_on_fail)
	{
		VmaAllocation vma_alloc;
		VkMemoryRequirements mem_req = {};
		VmaAllocationCreateInfo create_info = {};
		VkResult error_code = VK_ERROR_UNKNOWN;

		auto do_vma_alloc = [&]() -> std::tuple<VkResult, u32>
		{
			for (const auto& memory_type_index : memory_type)
			{
				mem_req.memoryTypeBits = 1u << memory_type_index;
				mem_req.size = ::align2(block_sz, alignment);
				mem_req.alignment = alignment;
				create_info.memoryTypeBits = 1u << memory_type_index;
				create_info.flags = m_allocation_flags;

				error_code = vmaAllocateMemory(m_allocator, &mem_req, &create_info, &vma_alloc, nullptr);
				if (error_code == VK_SUCCESS)
				{
					return { VK_SUCCESS, memory_type_index };
				}
			}

			return { error_code, ~0u };
		};

		// On successful allocation, simply tag the transaction and carry on.
		{
			const auto [status, type] = do_vma_alloc();
			if (status == VK_SUCCESS)
			{
				vmm_notify_memory_allocated(vma_alloc, type, block_sz, pool);
				return vma_alloc;
			}
		}

		const auto severity = (throw_on_fail) ? rsx::problem_severity::fatal : rsx::problem_severity::severe;
		if (error_code == VK_ERROR_OUT_OF_DEVICE_MEMORY &&
			vmm_handle_memory_pressure(severity))
		{
			// Out of memory. Try again.
			const auto [status, type] = do_vma_alloc();
			if (status == VK_SUCCESS)
			{
				rsx_log.warning("Renderer ran out of video memory but successfully recovered.");
				vmm_notify_memory_allocated(vma_alloc, type, block_sz, pool);
				return vma_alloc;
			}
		}

		if (!throw_on_fail)
		{
			return VK_NULL_HANDLE;
		}

		die_with_error(error_code);
		fmt::throw_exception("Unreachable! Error_code=0x%x", static_cast<u32>(error_code));
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
		if (!mem_handle)
		{
			return VK_NULL_HANDLE;
		}

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
		vmaGetHeapBudgets(m_allocator, stats.data());

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

	void mem_allocator_vma::set_safest_allocation_flags()
	{
		m_allocation_flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;
	}

	void mem_allocator_vma::set_fastest_allocation_flags()
	{
		m_allocation_flags = VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT;
	}

	mem_allocator_vk::mem_handle_t mem_allocator_vk::alloc(u64 block_sz, u64 /*alignment*/, const memory_type_info& memory_type, vmm_allocation_pool pool, bool throw_on_fail)
	{
		VkResult error_code = VK_ERROR_UNKNOWN;
		VkDeviceMemory memory;

		VkMemoryAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		info.allocationSize = block_sz;

		auto do_vk_alloc = [&]() -> std::tuple<VkResult, u32>
		{
			for (const auto& memory_type_index : memory_type)
			{
				info.memoryTypeIndex = memory_type_index;
				error_code = vkAllocateMemory(m_device, &info, nullptr, &memory);
				if (error_code == VK_SUCCESS)
				{
					return { error_code, memory_type_index };
				}
			}

			return { error_code, ~0u };
		};

		{
			const auto [status, type] = do_vk_alloc();
			if (status == VK_SUCCESS)
			{
				vmm_notify_memory_allocated(memory, type, block_sz, pool);
				return memory;
			}
		}

		const auto severity = (throw_on_fail) ? rsx::problem_severity::fatal : rsx::problem_severity::severe;
		if (error_code == VK_ERROR_OUT_OF_DEVICE_MEMORY &&
			vmm_handle_memory_pressure(severity))
		{
			// Out of memory. Try again.
			const auto [status, type] = do_vk_alloc();
			if (status == VK_SUCCESS)
			{
				rsx_log.warning("Renderer ran out of video memory but successfully recovered.");
				vmm_notify_memory_allocated(memory, type, block_sz, pool);
				return memory;
			}
		}

		if (!throw_on_fail)
		{
			return VK_NULL_HANDLE;
		}

		die_with_error(error_code);
		fmt::throw_exception("Unreachable! Error_code=0x%x", static_cast<u32>(error_code));
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

	memory_block::memory_block(VkDevice dev, u64 block_sz, u64 alignment, const memory_type_info& memory_type, vmm_allocation_pool pool, bool nullable)
		: m_device(dev), m_size(block_sz)
	{
		m_mem_allocator = get_current_mem_allocator();
		m_mem_handle    = m_mem_allocator->alloc(block_sz, alignment, memory_type, pool, !nullable);
	}

	memory_block::~memory_block()
	{
		if (m_mem_allocator && m_mem_handle)
		{
			m_mem_allocator->free(m_mem_handle);
		}
	}

	memory_block_host::memory_block_host(VkDevice dev, void* host_pointer, u64 size, const memory_type_info& memory_type) :
		m_device(dev), m_mem_handle(VK_NULL_HANDLE), m_host_pointer(host_pointer)
	{
		VkMemoryAllocateInfo alloc_info{};
		VkImportMemoryHostPointerInfoEXT import_info{};

		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.memoryTypeIndex = memory_type.first();
		alloc_info.allocationSize = size;
		alloc_info.pNext = &import_info;

		import_info.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT;
		import_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
		import_info.pHostPointer = host_pointer;

		CHECK_RESULT(vkAllocateMemory(m_device, &alloc_info, nullptr, &m_mem_handle));
	}

	memory_block_host::~memory_block_host()
	{
		vkFreeMemory(m_device, m_mem_handle, nullptr);
	}

	VkDeviceMemory memory_block_host::get_vk_device_memory()
	{
		return m_mem_handle;
	}

	u64 memory_block_host::get_vk_device_memory_offset()
	{
		return 0ull;
	}

	void* memory_block_host::map(u64 offset, u64 /*size*/)
	{
		return reinterpret_cast<char*>(m_host_pointer) + offset;
	}

	void memory_block_host::unmap()
	{
		// NOP
	}

	VkDeviceMemory memory_block::get_vk_device_memory()
	{
		return m_mem_allocator->get_vk_device_memory(m_mem_handle);
	}

	u64 memory_block::get_vk_device_memory_offset()
	{
		return m_mem_allocator->get_vk_device_memory_offset(m_mem_handle);
	}

	u64 memory_block::size() const
	{
		return m_size;
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
