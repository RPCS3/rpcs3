#pragma once

#include "../VulkanAPI.h"
#include "Utilities/mutex.h"

#include "commands.h"
#include "device.h"

#include "Emu/RSX/Common/simple_array.hpp"

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

		VkDescriptorSet allocate(VkDescriptorSetLayout layout, VkBool32 use_cache, u32 used_count);

		operator VkDescriptorPool() { return m_current_pool_handle; }
		FORCE_INLINE bool valid() const { return (!m_device_pools.empty()); }
		FORCE_INLINE u32 max_sets() const { return info.maxSets; }
		FORCE_INLINE bool can_allocate(u32 required_count, u32 used_count) const { return (used_count + required_count) <= info.maxSets; };

	private:
		const vk::render_device* m_owner = nullptr;
		VkDescriptorPoolCreateInfo info = {};

		rsx::simple_array<VkDescriptorPool> m_device_pools;
		VkDescriptorPool m_current_pool_handle = VK_NULL_HANDLE;
		u32 m_current_pool_index = 0;

		static constexpr size_t max_cache_size = 64;
		VkDescriptorSetLayout m_cached_layout = VK_NULL_HANDLE;
		rsx::simple_array<VkDescriptorSet> m_descriptor_set_cache;
		rsx::simple_array<VkDescriptorSetLayout> m_allocation_request_cache;
	};

	class descriptor_set
	{
		static constexpr size_t max_cache_size = 16384;
		static constexpr size_t max_overflow_size = 64;
		static constexpr size_t m_pool_size = max_cache_size + max_overflow_size;

		void init(VkDescriptorSet new_set);

	public:
		descriptor_set(VkDescriptorSet set);
		descriptor_set() = default;
		~descriptor_set();

		descriptor_set(const descriptor_set&) = delete;

		void swap(descriptor_set& other);
		descriptor_set& operator = (VkDescriptorSet set);

		VkDescriptorSet* ptr();
		VkDescriptorSet value() const;
		void push(const VkBufferView& buffer_view, VkDescriptorType type, u32 binding);
		void push(const VkDescriptorBufferInfo& buffer_info, VkDescriptorType type, u32 binding);
		void push(const VkDescriptorImageInfo& image_info, VkDescriptorType type, u32 binding);
		void push(const VkDescriptorImageInfo* image_info, u32 count, VkDescriptorType type, u32 binding);
		void push(rsx::simple_array<VkCopyDescriptorSet>& copy_cmd, u32 type_mask = umax);

		void bind(VkCommandBuffer cmd, VkPipelineBindPoint bind_point, VkPipelineLayout layout);
		void bind(const command_buffer& cmd, VkPipelineBindPoint bind_point, VkPipelineLayout layout);

		void flush();

	private:
		VkDescriptorSet m_handle = VK_NULL_HANDLE;
		u64 m_update_after_bind_mask = 0;
		u64 m_push_type_mask = 0;
		bool m_in_use = false;

		rsx::simple_array<VkBufferView> m_buffer_view_pool;
		rsx::simple_array<VkDescriptorBufferInfo> m_buffer_info_pool;
		rsx::simple_array<VkDescriptorImageInfo> m_image_info_pool;

		rsx::simple_array<VkWriteDescriptorSet> m_pending_writes;
		rsx::simple_array<VkCopyDescriptorSet> m_pending_copies;
	};

	namespace descriptors
	{
		void init();
		void flush();

		VkDescriptorSetLayout create_layout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
	}
}
