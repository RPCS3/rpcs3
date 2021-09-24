#pragma once

#include "../VulkanAPI.h"
#include "commands.h"
#include "device.h"

#include "Emu/RSX/rsx_utils.h"

namespace vk
{
	class descriptor_pool
	{
	public:
		descriptor_pool() = default;
		~descriptor_pool() = default;

		void create(const vk::render_device& dev, VkDescriptorPoolSize* sizes, u32 size_descriptors_count, u32 max_sets, u8 subpool_count);
		void destroy();
		void reset(VkDescriptorPoolResetFlags flags);

		bool valid() const;
		operator VkDescriptorPool();

	private:
		const vk::render_device* m_owner = nullptr;

		rsx::simple_array<VkDescriptorPool> m_device_pools;
		VkDescriptorPool m_current_pool_handle = VK_NULL_HANDLE;
		u32 m_current_pool_index = 0;
	};

	class descriptor_set
	{
		const size_t max_cache_size = 16384;

		void flush();
		void init();

	public:
		descriptor_set(VkDescriptorSet set);
		descriptor_set();
		~descriptor_set() = default;

		descriptor_set(const descriptor_set&) = delete;

		void swap(descriptor_set& other);
		descriptor_set& operator = (VkDescriptorSet set);

		VkDescriptorSet* ptr();
		VkDescriptorSet value() const;
		void push(const VkBufferView& buffer_view, VkDescriptorType type, u32 binding);
		void push(const VkDescriptorBufferInfo& buffer_info, VkDescriptorType type, u32 binding);
		void push(const VkDescriptorImageInfo& image_info, VkDescriptorType type, u32 binding);
		void push(const VkDescriptorImageInfo* image_info, u32 count, VkDescriptorType type, u32 binding);
		void push(rsx::simple_array<VkCopyDescriptorSet>& copy_cmd);

		void bind(VkCommandBuffer cmd, VkPipelineBindPoint bind_point, VkPipelineLayout layout);
		void bind(const command_buffer& cmd, VkPipelineBindPoint bind_point, VkPipelineLayout layout);

	private:
		VkDescriptorSet m_handle = VK_NULL_HANDLE;

		rsx::simple_array<VkBufferView> m_buffer_view_pool;
		rsx::simple_array<VkDescriptorBufferInfo> m_buffer_info_pool;
		rsx::simple_array<VkDescriptorImageInfo> m_image_info_pool;

		rsx::simple_array<VkWriteDescriptorSet> m_pending_writes;
		rsx::simple_array<VkCopyDescriptorSet> m_pending_copies;
	};

	void flush_descriptor_updates();
}
