#include "event.h"
#include <memory>

namespace rpcs3
{
	inline namespace core
	{
		void event_manager::init()
		{
		}

		void event_manager::clear()
		{
			m_map.clear();
		}

		bool event_manager::unregister_key(u64 key)
		{
			if (!key)
			{
				// always ok
				return true;
			}

			std::lock_guard<std::mutex> lock(m_mutex);

			auto f = m_map.find(key);
			if (f != m_map.end())
			{
				m_map.erase(f);
				return true;
			}

			return false;
		}

		std::shared_ptr<lv2_event_queue_t> event_manager::get_event_queue(u64 key)
		{
			if (!key)
			{
				// never exists
				return nullptr;
			}

			std::lock_guard<std::mutex> lock(m_mutex);

			auto f = m_map.find(key);
			if (f != m_map.end())
			{
				return f->second;
			}

			return nullptr;
		}
	}
}