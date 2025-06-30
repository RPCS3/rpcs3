#include "Emu/IdManager.h"
#include "descriptors.h"
#include "garbage_collector.h"

namespace vk
{
	// Error handler callback
	extern void on_descriptor_pool_fragmentation(bool fatal);

	namespace descriptors
	{
		class dispatch_manager
		{
		public:
			inline void flush_all()
			{
				std::lock_guard lock(m_notifications_lock);

				for (auto& set : m_notification_list)
				{
					set->flush();
				}

				m_notification_list.clear();
			}

			void register_(descriptor_set* set)
			{
				std::lock_guard lock(m_notifications_lock);

				m_notification_list.push_back(set);
				// rsx_log.notice("[descriptor_manager::register] Now monitoring %u descriptor sets", m_notification_list.size());
			}

			void deregister(descriptor_set* set)
			{
				std::lock_guard lock(m_notifications_lock);

				m_notification_list.erase_if(FN(x == set));
				// rsx_log.notice("[descriptor_manager::deregister] Now monitoring %u descriptor sets", m_notification_list.size());
			}

			void destroy()
			{
				std::lock_guard lock(m_notifications_lock);
				m_notification_list.clear();
			}

			dispatch_manager() = default;

		private:
			rsx::simple_array<descriptor_set*> m_notification_list;
			std::mutex m_notifications_lock;

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

		void destroy()
		{
			g_fxo->get<dispatch_manager>().destroy();
		}

		VkDescriptorSetLayout create_layout(const rsx::simple_array<VkDescriptorSetLayoutBinding>& bindings)
		{
			VkDescriptorSetLayoutCreateInfo infos = {};
			infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			infos.pBindings = bindings.data();
			infos.bindingCount = ::size32(bindings);

			VkDescriptorSetLayoutBindingFlagsCreateInfo binding_infos = {};
			rsx::simple_array<VkDescriptorBindingFlags> binding_flags;

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
					binding_flags[i] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
				}
			}

			binding_infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			binding_infos.pNext = nullptr;
			binding_infos.bindingCount = ::size32(binding_flags);
			binding_infos.pBindingFlags = binding_flags.data();

			infos.pNext = &binding_infos;
			infos.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

			VkDescriptorSetLayout result;
			CHECK_RESULT(vkCreateDescriptorSetLayout(*g_render_device, &infos, nullptr, &result));
			return result;
		}
	}

	void descriptor_pool::create(const vk::render_device& dev, const rsx::simple_array<VkDescriptorPoolSize>& pool_sizes, u32 max_sets)
	{
		ensure(max_sets > 16);

		m_create_info_pool_sizes = pool_sizes;

		for (auto& size : m_create_info_pool_sizes)
		{
			ensure(size.descriptorCount < 128); // Sanity check. Remove before commit.
			size.descriptorCount *= max_sets;
		}

		m_create_info.flags = dev.get_descriptor_update_after_bind_support() ? VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT : 0;
		m_create_info.maxSets = max_sets;
		m_create_info.poolSizeCount = m_create_info_pool_sizes.size();
		m_create_info.pPoolSizes = m_create_info_pool_sizes.data();
		m_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

		m_owner = &dev;
		next_subpool();
	}

	void descriptor_pool::destroy()
	{
		if (m_device_subpools.empty()) return;

		for (auto& pool : m_device_subpools)
		{
			vkDestroyDescriptorPool((*m_owner), pool.handle, nullptr);
			pool.handle = VK_NULL_HANDLE;
		}

		m_owner = nullptr;
	}

	void descriptor_pool::reset(u32 subpool_id, VkDescriptorPoolResetFlags flags)
	{
		std::lock_guard lock(m_subpool_lock);

		CHECK_RESULT(vkResetDescriptorPool(*m_owner, m_device_subpools[subpool_id].handle, flags));
		m_device_subpools[subpool_id].busy = VK_FALSE;
	}

	VkDescriptorSet descriptor_pool::allocate(VkDescriptorSetLayout layout, VkBool32 use_cache)
	{
		if (use_cache)
		{
			if (m_descriptor_set_cache.empty())
			{
				// For optimal cache utilization, each pool should only allocate one layout
				m_cached_layout = layout;
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

		if (!can_allocate(use_cache ? 4 : 1, m_current_subpool_offset))
		{
			next_subpool();
		}

		VkDescriptorSet new_descriptor_set;
		VkDescriptorSetAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = m_current_pool_handle;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = &layout;

		if (use_cache)
		{
			const auto alloc_size = std::min<u32>(m_create_info.maxSets - m_current_subpool_offset, max_cache_size);
			m_allocation_request_cache.resize(alloc_size);
			for (auto& layout_ : m_allocation_request_cache)
			{
				layout_ = m_cached_layout;
			}

			ensure(m_descriptor_set_cache.empty());
			alloc_info.descriptorSetCount = alloc_size;
			alloc_info.pSetLayouts = m_allocation_request_cache.data();

			m_descriptor_set_cache.resize(alloc_size);
			CHECK_RESULT(vkAllocateDescriptorSets(*m_owner, &alloc_info, m_descriptor_set_cache.data()));

			m_current_subpool_offset += alloc_size;
			new_descriptor_set = m_descriptor_set_cache.pop_back();
		}
		else
		{
			m_current_subpool_offset++;
			CHECK_RESULT(vkAllocateDescriptorSets(*m_owner, &alloc_info, &new_descriptor_set));
		}

		return new_descriptor_set;
	}

	void descriptor_pool::next_subpool()
	{
		if (m_current_subpool_index != umax)
		{
			// Enqueue release using gc
			auto release_func = [subpool_index=m_current_subpool_index, this]()
			{
				this->reset(subpool_index, 0);
			};

			auto cleanup_obj = std::make_unique<gc_callback_t>(release_func);
			vk::get_gc()->dispose(cleanup_obj);
		}

		m_current_subpool_offset = 0;
		m_current_subpool_index = umax;

		const int max_retries = 2;
		int retries = max_retries;

		do
		{
			for (u32 index = 0; index < m_device_subpools.size(); ++index)
			{
				if (!m_device_subpools[index].busy)
				{
					m_current_subpool_index = index;
					goto done; // Nested break
				}
			}

			VkDescriptorPool subpool = VK_NULL_HANDLE;
			if (VkResult result = vkCreateDescriptorPool(*m_owner, &m_create_info, nullptr, &subpool))
			{
				if (retries-- && (result == VK_ERROR_FRAGMENTATION_EXT))
				{
					rsx_log.warning("Descriptor pool creation failed with fragmentation error. Will attempt to recover.");
					vk::on_descriptor_pool_fragmentation(!retries);
					continue;
				}

				vk::die_with_error(result);
				fmt::throw_exception("Unreachable");
			}

			// New subpool created successfully
			std::lock_guard lock(m_subpool_lock);

			m_device_subpools.push_back(
			{
				.handle = subpool,
				.busy = VK_FALSE
			});

			m_current_subpool_index = m_device_subpools.size() - 1;

		} while (m_current_subpool_index == umax);

	done:
		m_device_subpools[m_current_subpool_index].busy = VK_TRUE;
		m_current_pool_handle = m_device_subpools[m_current_subpool_index].handle;
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

	void descriptor_set::push(const VkBufferView& buffer_view, VkDescriptorType type, u32 binding)
	{
		m_push_type_mask |= (1ull << type);
		m_buffer_view_pool.push_back(buffer_view);
		m_pending_writes.emplace_back(
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
		);
	}

	void descriptor_set::push(const VkDescriptorBufferInfo& buffer_info, VkDescriptorType type, u32 binding)
	{
		m_push_type_mask |= (1ull << type);
		m_buffer_info_pool.push_back(buffer_info);
		m_pending_writes.emplace_back(
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
		);
	}

	void descriptor_set::push(const VkDescriptorImageInfo& image_info, VkDescriptorType type, u32 binding)
	{
		m_push_type_mask |= (1ull << type);
		m_image_info_pool.push_back(image_info);
		m_pending_writes.emplace_back(
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
		);
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
			return;
		}

		m_pending_copies += copy_cmd;
	}

	void descriptor_set::push(rsx::simple_array<VkWriteDescriptorSet>& write_cmds, u32 type_mask)
	{
		m_push_type_mask |= type_mask;

#if !defined(__clang__) || (__clang_major__ >= 16)
		if (m_pending_writes.empty()) [[unlikely]]
		{
			m_pending_writes = std::move(write_cmds);
			return;
		}
#endif
		m_pending_writes += write_cmds;
	}

	void descriptor_set::push(const descriptor_set_dynamic_offset_t& offset)
	{
		ensure(offset.location >= 0 && offset.location <= 16);
		while (m_dynamic_offsets.size() < (static_cast<u32>(offset.location) + 1u))
		{
			m_dynamic_offsets.push_back(0);
		}

		m_dynamic_offsets[offset.location] = offset.value;
	}

	void descriptor_set::on_bind()
	{
		if (!m_push_type_mask)
		{
			ensure(m_pending_writes.empty());
			return;
		}

		// We have queued writes
		if ((m_push_type_mask & ~m_update_after_bind_mask) ||
			(m_pending_writes.size() >= max_cache_size))
		{
			flush();
			return;
		}

		// Register for async flush
		ensure(m_update_after_bind_mask);
		g_fxo->get<descriptors::dispatch_manager>().register_(this);
	}

	void descriptor_set::bind(const vk::command_buffer& cmd, VkPipelineBindPoint bind_point, VkPipelineLayout layout)
	{
		// Notify
		on_bind();

		vkCmdBindDescriptorSets(cmd, bind_point, layout, 0, 1, &m_handle, ::size32(m_dynamic_offsets), m_dynamic_offsets.data());
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
