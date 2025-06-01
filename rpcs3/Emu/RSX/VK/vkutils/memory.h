#pragma once

#include "../VulkanAPI.h"
#include "../../rsx_utils.h"
#include "shared.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#endif
#define VMA_VULKAN_VERSION 1002000
#include "3rdparty/GPUOpen/VulkanMemoryAllocator/include/vk_mem_alloc.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace vk
{
	namespace vmm_allocation_pool_ // Workaround for clang < 13 not supporting enum imports
	{
		enum vmm_allocation_pool
		{
			VMM_ALLOCATION_POOL_UNDEFINED = 0,
			VMM_ALLOCATION_POOL_SYSTEM,
			VMM_ALLOCATION_POOL_SURFACE_CACHE,
			VMM_ALLOCATION_POOL_TEXTURE_CACHE,
			VMM_ALLOCATION_POOL_SWAPCHAIN,
			VMM_ALLOCATION_POOL_SCRATCH,
			VMM_ALLOCATION_POOL_SAMPLER,
		};
	}

	using namespace vk::vmm_allocation_pool_;

	class render_device;
	class memory_type_info
	{
		std::vector<u32> type_ids;
		std::vector<u64> type_sizes;

	public:
		memory_type_info() = default;
		memory_type_info(u32 index, u64 size);
		void push(u32 index, u64 size);

		using iterator = u32*;
		using const_iterator = const u32*;
		const_iterator begin() const;
		const_iterator end() const;
		u32 first() const;
		size_t count() const;

		operator bool() const;
		bool operator == (const memory_type_info& other) const;

		memory_type_info get(const render_device& dev, u32 access_flags, u32 type_mask) const;

		void rebalance();
	};

	class mem_allocator_base
	{
	public:
		using mem_handle_t = void*;

		mem_allocator_base(const vk::render_device& dev, VkPhysicalDevice pdev);
		virtual ~mem_allocator_base() = default;

		virtual void destroy() = 0;

		virtual mem_handle_t alloc(u64 block_sz, u64 alignment, const memory_type_info& memory_type, vmm_allocation_pool pool, bool throw_on_fail) = 0;
		virtual void free(mem_handle_t mem_handle) = 0;
		virtual void* map(mem_handle_t mem_handle, u64 offset, u64 size) = 0;
		virtual void unmap(mem_handle_t mem_handle) = 0;
		virtual VkDeviceMemory get_vk_device_memory(mem_handle_t mem_handle) = 0;
		virtual u64 get_vk_device_memory_offset(mem_handle_t mem_handle) = 0;
		virtual f32 get_memory_usage() = 0;

		virtual void set_safest_allocation_flags() {}
		virtual void set_fastest_allocation_flags() {}

	protected:
		VkDevice m_device;
		VkFlags  m_allocation_flags;
	};


	// Memory Allocator - Vulkan Memory Allocator
	// https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator

	class mem_allocator_vma : public mem_allocator_base
	{
	public:
		mem_allocator_vma(const vk::render_device& dev, VkPhysicalDevice pdev, VkInstance inst);
		~mem_allocator_vma() override = default;

		void destroy() override;

		mem_handle_t alloc(u64 block_sz, u64 alignment, const memory_type_info& memory_type, vmm_allocation_pool pool, bool throw_on_fail) override;

		void free(mem_handle_t mem_handle) override;
		void* map(mem_handle_t mem_handle, u64 offset, u64 /*size*/) override;
		void unmap(mem_handle_t mem_handle) override;

		VkDeviceMemory get_vk_device_memory(mem_handle_t mem_handle) override;
		u64 get_vk_device_memory_offset(mem_handle_t mem_handle) override;
		f32 get_memory_usage() override;

		void set_safest_allocation_flags() override;
		void set_fastest_allocation_flags() override;

	private:
		VmaAllocator m_allocator;
		std::array<VmaBudget, VK_MAX_MEMORY_HEAPS> stats;
	};


	// Memory Allocator - built-in Vulkan device memory allocate/free

	class mem_allocator_vk : public mem_allocator_base
	{
	public:
		mem_allocator_vk(const vk::render_device& dev, VkPhysicalDevice pdev) : mem_allocator_base(dev, pdev) {}
		~mem_allocator_vk() override = default;

		void destroy() override {}

		mem_handle_t alloc(u64 block_sz, u64 /*alignment*/, const memory_type_info& memory_type, vmm_allocation_pool pool, bool throw_on_fail) override;

		void free(mem_handle_t mem_handle) override;
		void* map(mem_handle_t mem_handle, u64 offset, u64 size) override;
		void unmap(mem_handle_t mem_handle) override;

		VkDeviceMemory get_vk_device_memory(mem_handle_t mem_handle) override;
		u64 get_vk_device_memory_offset(mem_handle_t /*mem_handle*/) override;
		f32 get_memory_usage() override;
	};

	struct memory_block
	{
		memory_block(VkDevice dev, u64 block_sz, u64 alignment, const memory_type_info& memory_type, vmm_allocation_pool pool, bool nullable = false);
		virtual ~memory_block();

		virtual VkDeviceMemory get_vk_device_memory();
		virtual u64 get_vk_device_memory_offset();

		virtual void* map(u64 offset, u64 size);
		virtual void unmap();

		u64 size() const;

		memory_block(const memory_block&) = delete;
		memory_block(memory_block&&)      = delete;

	protected:
		memory_block() = default;

	private:
		VkDevice m_device;
		vk::mem_allocator_base* m_mem_allocator = nullptr;
		mem_allocator_base::mem_handle_t m_mem_handle;
		u64 m_size;
	};

	struct memory_block_host : public memory_block
	{
		memory_block_host(VkDevice dev, void* host_pointer, u64 size, const memory_type_info& memory_type);
		~memory_block_host();

		VkDeviceMemory get_vk_device_memory() override;
		u64 get_vk_device_memory_offset() override;
		void* map(u64 offset, u64 size) override;
		void unmap() override;

		memory_block_host(const memory_block_host&) = delete;
		memory_block_host(memory_block_host&&) = delete;
		memory_block_host() = delete;

	private:
		VkDevice m_device;
		VkDeviceMemory m_mem_handle;
		void* m_host_pointer;
	};

	// Tracking for memory usage. Constrained largely by amount of VRAM + shared video memory.
	void vmm_notify_memory_allocated(void* handle, u32 memory_type, u64 memory_size, vmm_allocation_pool pool);
	void vmm_notify_memory_freed(void* handle);
	void vmm_reset();
	void vmm_check_memory_usage();
	u64  vmm_get_application_memory_usage(const memory_type_info& memory_type);
	u64  vmm_get_application_pool_usage(vmm_allocation_pool pool);
	bool vmm_handle_memory_pressure(rsx::problem_severity severity);
	rsx::problem_severity vmm_determine_memory_load_severity();

	// Tracking for host memory objects. Allocated count is more important than actual memory amount.
	void vmm_notify_object_allocated(vmm_allocation_pool pool);
	void vmm_notify_object_freed(vmm_allocation_pool pool);

	mem_allocator_base* get_current_mem_allocator();
}
