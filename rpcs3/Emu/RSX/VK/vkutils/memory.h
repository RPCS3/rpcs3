#pragma once

#include "../VulkanAPI.h"
#include "../../rsx_utils.h"
#include "shared.h"

#include "3rdparty/GPUOpen/include/vk_mem_alloc.h"

namespace vk
{
	class mem_allocator_base
	{
	public:
		using mem_handle_t = void*;

		mem_allocator_base(VkDevice dev, VkPhysicalDevice /*pdev*/) : m_device(dev) {}
		virtual ~mem_allocator_base() = default;

		virtual void destroy() = 0;

		virtual mem_handle_t alloc(u64 block_sz, u64 alignment, u32 memory_type_index) = 0;
		virtual void free(mem_handle_t mem_handle) = 0;
		virtual void* map(mem_handle_t mem_handle, u64 offset, u64 size) = 0;
		virtual void unmap(mem_handle_t mem_handle) = 0;
		virtual VkDeviceMemory get_vk_device_memory(mem_handle_t mem_handle) = 0;
		virtual u64 get_vk_device_memory_offset(mem_handle_t mem_handle) = 0;
		virtual f32 get_memory_usage() = 0;

	protected:
		VkDevice m_device;
	};


	// Memory Allocator - Vulkan Memory Allocator
	// https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator

	class mem_allocator_vma : public mem_allocator_base
	{
	public:
		mem_allocator_vma(VkDevice dev, VkPhysicalDevice pdev);
		~mem_allocator_vma() override = default;

		void destroy() override;

		mem_handle_t alloc(u64 block_sz, u64 alignment, u32 memory_type_index) override;

		void free(mem_handle_t mem_handle) override;
		void* map(mem_handle_t mem_handle, u64 offset, u64 /*size*/) override;
		void unmap(mem_handle_t mem_handle) override;

		VkDeviceMemory get_vk_device_memory(mem_handle_t mem_handle) override;
		u64 get_vk_device_memory_offset(mem_handle_t mem_handle) override;
		f32 get_memory_usage() override;

	private:
		VmaAllocator m_allocator;
		std::array<VmaBudget, VK_MAX_MEMORY_HEAPS> stats;
	};


	// Memory Allocator - built-in Vulkan device memory allocate/free

	class mem_allocator_vk : public mem_allocator_base
	{
	public:
		mem_allocator_vk(VkDevice dev, VkPhysicalDevice pdev) : mem_allocator_base(dev, pdev) {}
		~mem_allocator_vk() override = default;

		void destroy() override {}

		mem_handle_t alloc(u64 block_sz, u64 /*alignment*/, u32 memory_type_index) override;

		void free(mem_handle_t mem_handle) override;
		void* map(mem_handle_t mem_handle, u64 offset, u64 size) override;
		void unmap(mem_handle_t mem_handle) override;

		VkDeviceMemory get_vk_device_memory(mem_handle_t mem_handle) override;
		u64 get_vk_device_memory_offset(mem_handle_t /*mem_handle*/) override;
		f32 get_memory_usage() override;
	};

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

	void vmm_notify_memory_allocated(void* handle, u32 memory_type, u64 memory_size);
	void vmm_notify_memory_freed(void* handle);
	void vmm_reset();
	void vmm_check_memory_usage();
	bool vmm_handle_memory_pressure(rsx::problem_severity severity);

	mem_allocator_base* get_current_mem_allocator();
}
