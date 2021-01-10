#include "stdafx.h"
#include "VKQueryPool.h"
#include "VKRenderPass.h"
#include "VKResourceManager.h"

namespace vk
{
	inline bool query_pool_manager::poke_query(query_slot_info& query, u32 index, VkQueryResultFlags flags)
	{
		// Query is ready if:
		// 1. Any sample has been determined to have passed the Z test
		// 2. The backend has fully processed the query and found no hits

		u32 result[2] = { 0, 0 };
		switch (const auto error = vkGetQueryPoolResults(*owner, *query.pool, index, 1, 8, result, 8, flags | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT))
		{
		case VK_SUCCESS:
		{
			if (result[0])
			{
				query.any_passed = true;
				query.ready = true;
				return true;
			}
			else if (result[1])
			{
				query.any_passed = false;
				query.ready = true;
				return true;
			}

			return false;
		}
		case VK_NOT_READY:
		{
			if (result[0])
			{
				query.any_passed = true;
				query.ready = true;
				return true;
			}

			return false;
		}
		default:
			die_with_error(error);
			return false;
		}
	}

	query_pool_manager::query_pool_manager(vk::render_device& dev, VkQueryType type, u32 num_entries)
	{
		ensure(num_entries > 0);

		owner = &dev;
		query_type = type;
		query_slot_status.resize(num_entries, {});

		for (unsigned i = 0; i < num_entries; ++i)
		{
			m_available_slots.push_back(i);
		}
	}

	query_pool_manager::~query_pool_manager()
	{
		if (m_current_query_pool)
		{
			m_current_query_pool.reset();
			owner = nullptr;
		}
	}

	void query_pool_manager::allocate_new_pool(vk::command_buffer& cmd)
	{
		ensure(!m_current_query_pool);

		const u32 count = ::size32(query_slot_status);
		m_current_query_pool = std::make_unique<query_pool>(*owner, query_type, count);

		// From spec: "After query pool creation, each query must be reset before it is used."
		vkCmdResetQueryPool(cmd, *m_current_query_pool.get(), 0, count);

		m_pool_lifetime_counter = count;
	}

	void query_pool_manager::reallocate_pool(vk::command_buffer& cmd)
	{
		if (m_current_query_pool)
		{
			if (!m_current_query_pool->has_refs())
			{
				vk::get_resource_manager()->dispose(m_current_query_pool);
			}
			else
			{
				m_consumed_pools.emplace_back(std::move(m_current_query_pool));

				// Sanity check
				if (m_consumed_pools.size() > 3)
				{
					rsx_log.error("[Robustness warning] Query pool discard pile size is now %llu. Are we leaking??", m_consumed_pools.size());
				}
			}
		}

		allocate_new_pool(cmd);
	}

	void query_pool_manager::run_pool_cleanup()
	{
		for (auto It = m_consumed_pools.begin(); It != m_consumed_pools.end();)
		{
			if (!(*It)->has_refs())
			{
				vk::get_resource_manager()->dispose(*It);
				It = m_consumed_pools.erase(It);
			}
			else
			{
				It++;
			}
		}
	}

	void query_pool_manager::begin_query(vk::command_buffer& cmd, u32 index)
	{
		ensure(query_slot_status[index].active == false);

		auto& query_info = query_slot_status[index];
		query_info.pool = m_current_query_pool.get();
		query_info.active = true;

		vkCmdBeginQuery(cmd, *query_info.pool, index, 0);//VK_QUERY_CONTROL_PRECISE_BIT);
	}

	void query_pool_manager::end_query(vk::command_buffer& cmd, u32 index)
	{
		vkCmdEndQuery(cmd, *query_slot_status[index].pool, index);
	}

	bool query_pool_manager::check_query_status(u32 index)
	{
		return poke_query(query_slot_status[index], index, VK_QUERY_RESULT_PARTIAL_BIT);
	}

	u32 query_pool_manager::get_query_result(u32 index)
	{
		// Check for cached result
		auto& query_info = query_slot_status[index];

		while (!query_info.ready)
		{
			poke_query(query_info, index, VK_QUERY_RESULT_PARTIAL_BIT);
		}

		return query_info.any_passed ? 1 : 0;
	}

	void query_pool_manager::get_query_result_indirect(vk::command_buffer& cmd, u32 index, VkBuffer dst, VkDeviceSize dst_offset)
	{
		vkCmdCopyQueryPoolResults(cmd, *query_slot_status[index].pool, index, 1, dst, dst_offset, 4, VK_QUERY_RESULT_WAIT_BIT);
	}

	void query_pool_manager::free_query(vk::command_buffer&/*cmd*/, u32 index)
	{
		// Release reference and discard
		auto& query = query_slot_status[index];

		ensure(query.active);
		query.pool->release();

		if (!query.pool->has_refs())
		{
			// No more refs held, remove if in discard pile
			run_pool_cleanup();
		}

		query = {};
		m_available_slots.push_back(index);
	}

	u32 query_pool_manager::allocate_query(vk::command_buffer& cmd)
	{
		if (!m_pool_lifetime_counter)
		{
			// Pool is exhaused, create a new one
			// This is basically a driver-level pool reset without synchronization
			// TODO: Alternatively, use VK_EXT_host_pool_reset to reset an old pool with no references and swap that in
			if (vk::is_renderpass_open(cmd))
			{
				vk::end_renderpass(cmd);
			}

			reallocate_pool(cmd);
		}

		if (!m_available_slots.empty())
		{
			m_pool_lifetime_counter--;

			const auto result = m_available_slots.front();
			m_available_slots.pop_front();
			return result;
		}

		return ~0u;
	}
}
