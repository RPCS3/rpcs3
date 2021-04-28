#pragma once
#include "VulkanAPI.h"
#include <deque>

namespace vk
{
	class command_buffer;
	class query_pool;
	class render_device;

	class query_pool_manager
	{
		struct query_slot_info
		{
			query_pool* pool;
			bool any_passed;
			bool active;
			bool ready;
		};

		std::vector<std::unique_ptr<query_pool>> m_consumed_pools;
		std::unique_ptr<query_pool> m_current_query_pool;
		std::deque<u32> m_available_slots;
		u32 m_pool_lifetime_counter = 0;
		VkQueryType query_type = VK_QUERY_TYPE_OCCLUSION;

		vk::render_device* owner = nullptr;
		std::vector<query_slot_info> query_slot_status;

		bool poke_query(query_slot_info& query, u32 index, VkQueryResultFlags flags);
		void allocate_new_pool(vk::command_buffer& cmd);
		void reallocate_pool(vk::command_buffer& cmd);
		void run_pool_cleanup();

	public:
		query_pool_manager(vk::render_device& dev, VkQueryType type, u32 num_entries);
		~query_pool_manager();

		void begin_query(vk::command_buffer& cmd, u32 index);
		void end_query(vk::command_buffer& cmd, u32 index);

		bool check_query_status(u32 index);
		u32  get_query_result(u32 index);
		void get_query_result_indirect(vk::command_buffer& cmd, u32 index, VkBuffer dst, VkDeviceSize dst_offset);

		u32 allocate_query(vk::command_buffer& cmd);
		void free_query(vk::command_buffer&/*cmd*/, u32 index);

		template<template<class> class _List>
		void free_queries(vk::command_buffer& cmd, _List<u32>& list)
		{
			for (const auto index : list)
			{
				free_query(cmd, index);
			}
		}
	};
};
