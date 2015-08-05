#include "stdafx.h"
#include "IdManager.h"

namespace idm
{
	std::mutex g_id_mutex;

	std::unordered_map<u32, ID_data_t> g_id_map;

	u32 g_cur_id = 1;

	void clear()
	{
		std::lock_guard<std::mutex> lock(g_id_mutex);

		g_id_map.clear();
		g_cur_id = 1; // first ID
	}
}
