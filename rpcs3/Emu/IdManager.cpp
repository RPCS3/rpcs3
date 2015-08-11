#include "stdafx.h"
#include "IdManager.h"

namespace idm
{
	std::mutex g_id_mutex;

	std::unordered_map<u32, id_data_t> g_id_map;

	thread_local u32 g_tls_last_id = 0xdeadbeef;

	u32 g_last_raw_id = 0;

	void clear()
	{
		std::lock_guard<std::mutex> lock(g_id_mutex);

		g_id_map.clear();
		g_last_raw_id = 0;
	}
}

namespace fxm
{
	std::mutex g_fx_mutex;

	std::unordered_map<std::type_index, std::shared_ptr<void>> g_fx_map;

	void clear()
	{
		std::lock_guard<std::mutex> lock(g_fx_mutex);

		g_fx_map.clear();
	}
}
