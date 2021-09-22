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

	VkBufferView* descriptor_set::dup(const VkBufferView* in)
	{
		if (!in) return nullptr;

		m_buffer_view_pool.push_back(*in);
		return &m_buffer_view_pool.back();
	}

	VkDescriptorBufferInfo* descriptor_set::dup(const VkDescriptorBufferInfo* in)
	{
		if (!in) return nullptr;

		m_buffer_info_pool.push_back(*in);
		return &m_buffer_info_pool.back();
	}

	VkDescriptorImageInfo* descriptor_set::dup(const VkDescriptorImageInfo* in)
	{
		if (!in) return nullptr;

		m_image_info_pool.push_back(*in);
		return &m_image_info_pool.back();
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

	void descriptor_set::push(const VkWriteDescriptorSet& write_cmd)
	{
		if (m_pending_writes.size() >= max_cache_size)
		{
			flush();
		}

		m_pending_writes.push_back(
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = m_handle,
			.dstBinding = write_cmd.dstBinding,
			.dstArrayElement = write_cmd.dstArrayElement,
			.descriptorCount = write_cmd.descriptorCount,
			.descriptorType = write_cmd.descriptorType,
			.pImageInfo = dup(write_cmd.pImageInfo),
			.pBufferInfo = dup(write_cmd.pBufferInfo),
			.pTexelBufferView = dup(write_cmd.pTexelBufferView)
		});
	}

	void descriptor_set::push(std::vector<VkCopyDescriptorSet>& copy_cmd)
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
