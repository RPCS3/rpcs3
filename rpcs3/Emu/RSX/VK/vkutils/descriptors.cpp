#include "descriptors.h"

namespace vk
{
	void descriptor_pool::create(const vk::render_device& dev, VkDescriptorPoolSize* sizes, u32 size_descriptors_count, u32 max_sets, u8 subpool_count)
	{
		ensure(subpool_count);

		VkDescriptorPoolCreateInfo infos = {};
		infos.flags = 0;
		infos.maxSets = max_sets;
		infos.poolSizeCount = size_descriptors_count;
		infos.pPoolSizes = sizes;
		infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

		m_owner = &dev;
		m_device_pools.resize(subpool_count);

		for (auto& pool : m_device_pools)
		{
			CHECK_RESULT(vkCreateDescriptorPool(dev, &infos, nullptr, &pool));
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

	bool descriptor_pool::valid() const
	{
		return (!m_device_pools.empty());
	}

	descriptor_pool::operator VkDescriptorPool()
	{
		return m_current_pool_handle;
	}

	void descriptor_pool::reset(VkDescriptorPoolResetFlags flags)
	{
		m_current_pool_index = (m_current_pool_index + 1) % u32(m_device_pools.size());
		m_current_pool_handle = m_device_pools[m_current_pool_index];
		CHECK_RESULT(vkResetDescriptorPool(*m_owner, m_current_pool_handle, flags));
	}

	descriptor_set::descriptor_set(VkDescriptorSet set)
	{
		flush();
		init();
		m_handle = set;
	}

	descriptor_set::descriptor_set()
	{
		init();
	}

	void descriptor_set::init()
	{
		if (m_image_info_pool.capacity() == 0)
		{
			m_image_info_pool.reserve(max_cache_size + 16);
			m_buffer_view_pool.reserve(max_cache_size + 16);
			m_buffer_info_pool.reserve(max_cache_size + 16);
		}
	}

	void descriptor_set::swap(descriptor_set& other)
	{
		other.flush();
		flush();

		std::swap(m_handle, other.m_handle);
	}

	descriptor_set& descriptor_set::operator = (VkDescriptorSet set)
	{
		flush();
		m_handle = set;
		return *this;
	}

	VkDescriptorSet* descriptor_set::ptr()
	{
		// TODO: You shouldn't need this
		// ensure(m_handle == VK_NULL_HANDLE);
		return &m_handle;
	}

	VkDescriptorSet descriptor_set::value() const
	{
		return m_handle;
	}

	void descriptor_set::push(const VkBufferView& buffer_view, VkDescriptorType type, u32 binding)
	{
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

	void descriptor_set::push(rsx::simple_array<VkCopyDescriptorSet>& copy_cmd)
	{
		if (m_pending_copies.empty()) [[likely]]
		{
			m_pending_copies = std::move(copy_cmd);
		}
		else
		{
			const size_t old_size = m_pending_copies.size();
			const size_t new_size = copy_cmd.size() + old_size;
			m_pending_copies.resize(new_size);
			std::copy(copy_cmd.begin(), copy_cmd.end(), m_pending_copies.begin() + old_size);
		}
	}

	void descriptor_set::bind(VkCommandBuffer cmd, VkPipelineBindPoint bind_point, VkPipelineLayout layout)
	{
		flush();
		vkCmdBindDescriptorSets(cmd, bind_point, layout, 0, 1, &m_handle, 0, nullptr);
	}

	void descriptor_set::bind(const command_buffer& cmd, VkPipelineBindPoint bind_point, VkPipelineLayout layout)
	{
		bind(static_cast<VkCommandBuffer>(cmd), bind_point, layout);
	}

	void descriptor_set::flush()
	{
		if (m_pending_writes.empty() && m_pending_copies.empty())
		{
			return;
		}

		const auto num_writes = ::size32(m_pending_writes);
		const auto num_copies = ::size32(m_pending_copies);
		vkUpdateDescriptorSets(*g_render_device, num_writes, m_pending_writes.data(), num_copies, m_pending_copies.data());

		m_pending_writes.clear();
		m_pending_copies.clear();
		m_image_info_pool.clear();
		m_buffer_info_pool.clear();
		m_buffer_view_pool.clear();
	}
}
