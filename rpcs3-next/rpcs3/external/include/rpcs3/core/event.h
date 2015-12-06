#pragma once

#include <common/basic_types.h>
#include <rpcs3/core/id_manager.h>
#include <mutex>
#include <unordered_map>

namespace rpcs3
{
	using namespace common;

	inline namespace core
	{
		struct lv2_event_queue_t;

		class event_manager
		{
			std::mutex m_mutex;
			std::unordered_map<u64, std::shared_ptr<lv2_event_queue_t>> m_map;

		public:
			void init();
			void clear();
			bool unregister_key(u64 key);

			template<typename... Args, typename = std::enable_if_t<std::is_constructible<lv2_event_queue_t, Args...>::value>>
			std::shared_ptr<lv2_event_queue_t> make_event_queue(u64 key, Args&&... args)
			{
				std::lock_guard<std::mutex> lock(m_mutex);

				if (key && m_map.find(key) != m_map.end())
				{
					return nullptr;
				}

				auto queue = idm::make_ptr<lv2_event_queue_t>(std::forward<Args>(args)...);

				if (key)
				{
					m_map[key] = queue;
				}

				return queue;
			}

			std::shared_ptr<lv2_event_queue_t> get_event_queue(u64 key);
		};
	}
}