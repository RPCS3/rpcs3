#include "id_manager.h"

namespace rpcs3
{
	inline namespace core
	{
		std::mutex idm::g_mutex;

		std::unordered_map<u32, id_data_t> idm::g_map;

		u32 idm::g_last_raw_id = 0;

		thread_local u32 idm::g_tls_last_id = 0xdeadbeef;

		std::mutex fxm::g_mutex;

		std::unordered_map<const void*, std::shared_ptr<void>> fxm::g_map;
	}
}
