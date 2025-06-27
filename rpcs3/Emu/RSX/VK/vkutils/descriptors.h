#pragma once

#include "../VulkanAPI.h"
#include "Utilities/mutex.h"

#include "commands.h"
#include "device.h"

#include "Emu/RSX/Common/simple_array.hpp"

namespace vk
{
	struct gc_callback_t
	{
		std::function<void()> m_callback;

		gc_callback_t(std::function<void()> callback)
			: m_callback(callback)
		{}

		~gc_callback_t()
		{
			if (m_callback)
			{
				m_callback();
			}
		}
	};

	struct descriptor_set_dynamic_offset_t
	{
		int location;
		u32 value;
	};

	class descriptor_pool
	{
	public:
		descriptor_pool() = default;
		~descriptor_pool() = default;

		void create(const vk::render_device& dev, const rsx::simple_array<VkDescriptorPoolSize>& pool_sizes, u32 max_sets = 1024);
		void destroy();

		VkDescriptorSet allocate(VkDescriptorSetLayout layout, VkBool32 use_cache = VK_TRUE);

		operator VkDescriptorPool() { return m_current_pool_handle; }
		FORCE_INLINE bool valid() const { return !m_device_subpools.empty(); }
		FORCE_INLINE u32 max_sets() const { return m_create_info.maxSets; }

	private:
		FORCE_INLINE bool can_allocate(u32 required_count, u32 already_used_count = 0) const { return (required_count + already_used_count) <= m_create_info.maxSets; };
		void reset(u32 subpool_id, VkDescriptorPoolResetFlags flags);
		void next_subpool();

		struct logical_subpool_t
		{
			VkDescriptorPool handle;
			VkBool32 busy;
		};

		const vk::render_device* m_owner = nullptr;
		VkDescriptorPoolCreateInfo m_create_info = {};
		rsx::simple_array<VkDescriptorPoolSize> m_create_info_pool_sizes;

		rsx::simple_array<logical_subpool_t> m_device_subpools;
		VkDescriptorPool m_current_pool_handle = VK_NULL_HANDLE;
		u32 m_current_subpool_index = umax;
		u32 m_current_subpool_offset = 0;

		shared_mutex m_subpool_lock;

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

		VkDescriptorSet value() const { return m_handle; }
		operator bool() const { return m_handle != VK_NULL_HANDLE; }

		VkDescriptorSet* ptr();
		void push(const VkBufferView& buffer_view, VkDescriptorType type, u32 binding);
		void push(const VkDescriptorBufferInfo& buffer_info, VkDescriptorType type, u32 binding);
		void push(const VkDescriptorImageInfo& image_info, VkDescriptorType type, u32 binding);
		void push(const VkDescriptorImageInfo* image_info, u32 count, VkDescriptorType type, u32 binding);
		void push(rsx::simple_array<VkCopyDescriptorSet>& copy_cmd, u32 type_mask = umax);
		void push(rsx::simple_array<VkWriteDescriptorSet>& write_cmds, u32 type_mask = umax);
		void push(const descriptor_set_dynamic_offset_t& offset);

		void on_bind();
		void bind(const vk::command_buffer& cmd, VkPipelineBindPoint bind_point, VkPipelineLayout layout);

		void flush();

	private:
		VkDescriptorSet m_handle = VK_NULL_HANDLE;
		u64 m_update_after_bind_mask = 0;
		u64 m_push_type_mask = 0;
		bool m_in_use = false;

		rsx::simple_array<VkBufferView> m_buffer_view_pool;
		rsx::simple_array<VkDescriptorBufferInfo> m_buffer_info_pool;
		rsx::simple_array<VkDescriptorImageInfo> m_image_info_pool;
		rsx::simple_array<u32> m_dynamic_offsets;

#if defined(__clang__) && (__clang_major__ < 16)
		// Clang (pre 16.x) does not support LWG 2089, std::construct_at for POD types
		struct WriteDescriptorSetT : public VkWriteDescriptorSet
		{
			WriteDescriptorSetT(
				VkStructureType                  sType,
				const void*                      pNext,
				VkDescriptorSet                  dstSet,
				uint32_t                         dstBinding,
				uint32_t                         dstArrayElement,
				uint32_t                         descriptorCount,
				VkDescriptorType                 descriptorType,
				const VkDescriptorImageInfo*     pImageInfo,
				const VkDescriptorBufferInfo*    pBufferInfo,
				const VkBufferView*              pTexelBufferView)
			{
				this->sType = sType,
				this->pNext = pNext,
				this->dstSet = dstSet,
				this->dstBinding = dstBinding,
				this->dstArrayElement = dstArrayElement,
				this->descriptorCount = descriptorCount,
				this->descriptorType = descriptorType,
				this->pImageInfo = pImageInfo,
				this->pBufferInfo = pBufferInfo,
				this->pTexelBufferView = pTexelBufferView;
			}
		};
#else
		using WriteDescriptorSetT = VkWriteDescriptorSet;
#endif

		rsx::simple_array<WriteDescriptorSetT> m_pending_writes;
		rsx::simple_array<VkCopyDescriptorSet> m_pending_copies;
	};

	namespace descriptors
	{
		void init();
		void flush();
		void destroy();

		VkDescriptorSetLayout create_layout(const rsx::simple_array<VkDescriptorSetLayoutBinding>& bindings);
	}
}
