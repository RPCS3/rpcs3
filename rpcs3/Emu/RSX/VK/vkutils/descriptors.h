#pragma once

#include "../VulkanAPI.h"
#include "commands.h"
#include "device.h"

#include <vector>

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

		std::vector<VkDescriptorPool> m_device_pools;
		VkDescriptorPool m_current_pool_handle = VK_NULL_HANDLE;
		u32 m_current_pool_index = 0;
	};

	class descriptor_set
	{
		const size_t max_cache_size = 16384;

		VkBufferView* dup(const VkBufferView* in);
		VkDescriptorBufferInfo* dup(const VkDescriptorBufferInfo* in);
		VkDescriptorImageInfo* dup(const VkDescriptorImageInfo* in);

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
		void push(const VkWriteDescriptorSet& write_cmd);
		void push(std::vector<VkCopyDescriptorSet>& copy_cmd);

		void bind(VkCommandBuffer cmd, VkPipelineBindPoint bind_point, VkPipelineLayout layout);
		void bind(const command_buffer& cmd, VkPipelineBindPoint bind_point, VkPipelineLayout layout);

	private:
		VkDescriptorSet m_handle = VK_NULL_HANDLE;

		std::vector<VkBufferView> m_buffer_view_pool;
		std::vector<VkDescriptorBufferInfo> m_buffer_info_pool;
		std::vector<VkDescriptorImageInfo> m_image_info_pool;

		std::vector<VkWriteDescriptorSet> m_pending_writes;
		std::vector<VkCopyDescriptorSet> m_pending_copies;
	};

	void flush_descriptor_updates();
}
