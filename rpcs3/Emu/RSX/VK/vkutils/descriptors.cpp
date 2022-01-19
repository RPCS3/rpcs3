#include "Emu/IdManager.h"
#include "descriptors.h"

namespace vk
{
	namespace descriptors
	{
		class dispatch_manager
		{
		public:
			inline void flush_all()
			{
				for (auto& set : m_notification_list)
				{
					set->flush();
				}
			}

			void register_(descriptor_set* set)
			{
				// Rare event, upon creation of a new set tracker.
				// Check for spurious 'new' events when the aux context is taking over
				for (const auto& set_ : m_notification_list)
				{
					if (set_ == set) return;
				}

				m_notification_list.push_back(set);
				rsx_log.warning("[descriptor_manager::register] Now monitoring %u descriptor sets", m_notification_list.size());
			}

			void deregister(descriptor_set* set)
			{
				for (auto it = m_notification_list.begin(); it != m_notification_list.end(); ++it)
				{
					if (*it == set)
					{
						*it = m_notification_list.back();
						m_notification_list.pop_back();
						break;
					}
				}

				rsx_log.warning("[descriptor_manager::deregister] Now monitoring %u descriptor sets", m_notification_list.size());
			}

			dispatch_manager() = default;

		private:
			rsx::simple_array<descriptor_set*> m_notification_list;

			dispatch_manager(const dispatch_manager&) = delete;
			dispatch_manager& operator = (const dispatch_manager&) = delete;
		};

		void init()
		{
			g_fxo->init<dispatch_manager>();
		}

		void flush()
		{
			g_fxo->get<dispatch_manager>().flush_all();
		}

		VkDescriptorSetLayout create_layout(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
		{
			VkDescriptorSetLayoutCreateInfo infos = {};
			infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			infos.pBindings = bindings.data();
			infos.bindingCount = ::size32(bindings);

			VkDescriptorSetLayoutBindingFlagsCreateInfo binding_infos = {};
			rsx::simple_array<VkDescriptorBindingFlags> binding_flags;

			if (g_render_device->get_descriptor_indexing_support())
			{
				const auto deferred_mask = g_render_device->get_descriptor_update_after_bind_support();
				binding_flags.resize(::size32(bindings));

				for (u32 i = 0; i < binding_flags.size(); ++i)
				{
					if ((1ull << bindings[i].descriptorType) & ~deferred_mask)
					{
						binding_flags[i] = 0u;
					}
					else
					{
						binding_flags[i] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
					}
				}

				binding_infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
				binding_infos.pNext = nullptr;
				binding_infos.bindingCount = ::size32(binding_flags);
				binding_infos.pBindingFlags = binding_flags.data();

				infos.pNext = &binding_infos;
				infos.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
			}

			VkDescriptorSetLayout result;
			CHECK_RESULT(vkCreateDescriptorSetLayout(*g_render_device, &infos, nullptr, &result));
			return result;
		}
	}

	void descriptor_pool::create(const vk::render_device& dev, VkDescriptorPoolSize* sizes, u32 size_descriptors_count, u32 max_sets, u8 subpool_count)
	{
		ensure(subpool_count);

		info.flags = dev.get_descriptor_update_after_bind_support() ? VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT : 0;
		info.maxSets = max_sets;
		info.poolSizeCount = size_descriptors_count;
		info.pPoolSizes = sizes;
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

		m_owner = &dev;
		m_device_pools.resize(subpool_count);

		for (auto& pool : m_device_pools)
		{
			CHECK_RESULT(vkCreateDescriptorPool(dev, &info, nullptr, &pool));
		}

		m_current_pool_handle = m_device_pools[0];
	}

	void descriptor_pool::destroy()
	{
		if (m_device_pools.empty()) return;

		for (auto& pool : m_device_pools)
		{
			vkDestroyDescriptorPool((*m_owner), pool, nullptr);
			pool = VK_NULL_HANDLE;
		}

		m_owner = nullptr;
	}

	void descriptor_pool::reset(VkDescriptorPoolResetFlags flags)
	{
		m_descriptor_set_cache.clear();
		m_current_pool_index = (m_current_pool_index + 1) % u32(m_device_pools.size());
		m_current_pool_handle = m_device_pools[m_current_pool_index];
		CHECK_RESULT(vkResetDescriptorPool(*m_owner, m_current_pool_handle, flags));
	}

	VkDescriptorSet descriptor_pool::allocate(VkDescriptorSetLayout layout, VkBool32 use_cache, u32 used_count)
	{
		if (use_cache)
		{
			if (m_descriptor_set_cache.empty())
			{
				// For optimal cache utilization, each pool should only allocate one layout
				if (m_cached_layout != layout)
				{
					m_cached_layout = layout;
					m_allocation_request_cache.resize(max_cache_size);

					for (auto& layout_ : m_allocation_request_cache)
					{
						layout_ = m_cached_layout;
					}
				}
			}
			else if (m_cached_layout != layout)
			{
				use_cache = VK_FALSE;
			}
			else
			{
				return m_descriptor_set_cache.pop_back();
			}
		}

		VkDescriptorSet new_descriptor_set;
		VkDescriptorSetAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = m_current_pool_handle;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = &layout;

		if (use_cache)
		{
			ensure(used_count < info.maxSets);
			const auto alloc_size = std::min<u32>(info.maxSets - used_count, max_cache_size);

			ensure(m_descriptor_set_cache.empty());
			alloc_info.descriptorSetCount = alloc_size;
			alloc_info.pSetLayouts = m_allocation_request_cache.data();

			m_descriptor_set_cache.resize(alloc_size);
			CHECK_RESULT(vkAllocateDescriptorSets(*m_owner, &alloc_info, m_descriptor_set_cache.data()));

			new_descriptor_set = m_descriptor_set_cache.pop_back();
		}
		else
		{
			CHECK_RESULT(vkAllocateDescriptorSets(*m_owner, &alloc_info, &new_descriptor_set));
		}

		return new_descriptor_set;
	}

	descriptor_set::descriptor_set(VkDescriptorSet set)
	{
		flush();
		m_handle = set;
	}

	descriptor_set::~descriptor_set()
	{
		if (m_update_after_bind_mask)
		{
			g_fxo->get<descriptors::dispatch_manager>().deregister(this);
		}
	}

	void descriptor_set::init(VkDescriptorSet new_set)
	{
		if (!m_in_use) [[unlikely]]
		{
			m_image_info_pool.reserve(m_pool_size);
			m_buffer_view_pool.reserve(m_pool_size);
			m_buffer_info_pool.reserve(m_pool_size);

			m_in_use = true;
			m_update_after_bind_mask = g_render_device->get_descriptor_update_after_bind_support();

			if (m_update_after_bind_mask)
			{
				g_fxo->get<descriptors::dispatch_manager>().register_(this);
			}
		}
		else if (m_push_type_mask & ~m_update_after_bind_mask)
		{
			flush();
		}

		m_handle = new_set;
	}

	void descriptor_set::swap(descriptor_set& other)
	{
		const auto other_handle = other.m_handle;
		other.flush();
		other.m_handle = m_handle;
		init(other_handle);
	}

	descriptor_set& descriptor_set::operator = (VkDescriptorSet set)
	{
		init(set);
		return *this;
	}

	VkDescriptorSet* descriptor_set::ptr()
	{
		if (!m_in_use) [[likely]]
		{
			init(m_handle);
		}

		return &m_handle;
	}

	VkDescriptorSet descriptor_set::value() const
	{
		return m_handle;
	}

	void descriptor_set::push(const VkBufferView& buffer_view, VkDescriptorType type, u32 binding)
	{
		m_push_type_mask |= (1ull << type);
		m_buffer_view_pool.push_back(buffer_view);
		m_pending_writes.push_back(
		{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
			nullptr,                                   // pNext
			m_handle,                                  // dstSet
			binding,                                   // dstBinding
			0,                                         // dstArrayElement
			1,                                         // descriptorCount
			type,                                      // descriptorType
			nullptr,                                   // pImageInfo
			nullptr,                                   // pBufferInfo
			&m_buffer_view_pool.back()                 // pTexelBufferView
		});
	}

	void descriptor_set::push(const VkDescriptorBufferInfo& buffer_info, VkDescriptorType type, u32 binding)
	{
		m_push_type_mask |= (1ull << type);
		m_buffer_info_pool.push_back(buffer_info);
		m_pending_writes.push_back(
		{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
			nullptr,                                   // pNext
			m_handle,                                  // dstSet
			binding,                                   // dstBinding
			0,                                         // dstArrayElement
			1,                                         // descriptorCount
			type,                                      // descriptorType
			nullptr,                                   // pImageInfo
			&m_buffer_info_pool.back(),                // pBufferInfo
			nullptr                                    // pTexelBufferView
		});
	}

	void descriptor_set::push(const VkDescriptorImageInfo& image_info, VkDescriptorType type, u32 binding)
	{
		m_push_type_mask |= (1ull << type);
		m_image_info_pool.push_back(image_info);
		m_pending_writes.push_back(
		{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
			nullptr,                                   // pNext
			m_handle,                                  // dstSet
			binding,                                   // dstBinding
			0,                                         // dstArrayElement
			1,                                         // descriptorCount
			type,                                      // descriptorType
			&m_image_info_pool.back(),                 // pImageInfo
			nullptr,                                   // pBufferInfo
			nullptr                                    // pTexelBufferView
		});
	}

	void descriptor_set::push(const VkDescriptorImageInfo* image_info, u32 count, VkDescriptorType type, u32 binding)
	{
		VkWriteDescriptorSet writer =
		{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
			nullptr,                                   // pNext
			m_handle,                                  // dstSet
			binding,                                   // dstBinding
			0,                                         // dstArrayElement
			count,                                     // descriptorCount
			type,                                      // descriptorType
			image_info,                                // pImageInfo
			nullptr,                                   // pBufferInfo
			nullptr                                    // pTexelBufferView
		};
		vkUpdateDescriptorSets(*g_render_device, 1, &writer, 0, nullptr);
	}

	void descriptor_set::push(rsx::simple_array<VkCopyDescriptorSet>& copy_cmd, u32 type_mask)
	{
		m_push_type_mask |= type_mask;

		if (m_pending_copies.empty()) [[likely]]
		{
			m_pending_copies = std::move(copy_cmd);
		}
		else
		{
			const auto old_size = m_pending_copies.size();
			const auto new_size = copy_cmd.size() + old_size;
			m_pending_copies.resize(new_size);
			std::copy(copy_cmd.begin(), copy_cmd.end(), m_pending_copies.begin() + old_size);
		}
	}

	void descriptor_set::bind(VkCommandBuffer cmd, VkPipelineBindPoint bind_point, VkPipelineLayout layout)
	{
		if ((m_push_type_mask & ~m_update_after_bind_mask) || (m_pending_writes.size() >= max_cache_size))
		{
			flush();
		}

		vkCmdBindDescriptorSets(cmd, bind_point, layout, 0, 1, &m_handle, 0, nullptr);
	}

	void descriptor_set::bind(const command_buffer& cmd, VkPipelineBindPoint bind_point, VkPipelineLayout layout)
	{
		bind(static_cast<VkCommandBuffer>(cmd), bind_point, layout);
	}

	void descriptor_set::flush()
	{
		if (!m_push_type_mask)
		{
			return;
		}

		const auto num_writes = ::size32(m_pending_writes);
		const auto num_copies = ::size32(m_pending_copies);
		vkUpdateDescriptorSets(*g_render_device, num_writes, m_pending_writes.data(), num_copies, m_pending_copies.data());

		m_push_type_mask = 0;
		m_pending_writes.clear();
		m_pending_copies.clear();
		m_image_info_pool.clear();
		m_buffer_info_pool.clear();
		m_buffer_view_pool.clear();
	}
}
