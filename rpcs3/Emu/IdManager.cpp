#include "stdafx.h"
#include "IdManager.h"

namespace idm
{
	shared_mutex g_mutex;

	idm::map_t g_map;

	u32 g_last_raw_id = 0;

	thread_local u32 g_tls_last_id = 0xdeadbeef;
}

namespace fxm
{
	shared_mutex g_mutex;

	fxm::map_t g_map;
}

void idm::clear()
{
	std::lock_guard<shared_mutex> lock(g_mutex);

	// Call recorded finalization functions for all IDs
	for (auto& id : idm::map_t(std::move(g_map)))
	{
		(*id.second.type_index)(id.second.data.get());
	}

	g_last_raw_id = 0;
}

bool idm::check(u32 in_id, id_type_index_t type)
{
	reader_lock lock(g_mutex);

	const auto found = g_map.find(in_id);

	return found != g_map.end() && found->second.type_index == type;
}

const std::type_info* idm::get_type(u32 raw_id)
{
	reader_lock lock(g_mutex);

	const auto found = g_map.find(raw_id);

	return found == g_map.end() ? nullptr : found->second.info;
}

std::shared_ptr<void> idm::get(u32 in_id, id_type_index_t type)
{
	reader_lock lock(g_mutex);

	const auto found = g_map.find(in_id);

	if (found == g_map.end() || found->second.type_index != type)
	{
		return nullptr;
	}

	return found->second.data;
}

idm::map_t idm::get_all(id_type_index_t type)
{
	reader_lock lock(g_mutex);

	idm::map_t result;

	for (auto& id : g_map)
	{
		if (id.second.type_index == type)
		{
			result.insert(id);
		}
	}

	return result;
}

std::shared_ptr<void> idm::withdraw(u32 in_id, id_type_index_t type)
{
	std::lock_guard<shared_mutex> lock(g_mutex);

	const auto found = g_map.find(in_id);

	if (found == g_map.end() || found->second.type_index != type)
	{
		return nullptr;
	}

	auto ptr = std::move(found->second.data);

	g_map.erase(found);

	return ptr;
}

u32 idm::get_count(id_type_index_t type)
{
	reader_lock lock(g_mutex);

	u32 result = 0;

	for (auto& id : g_map)
	{
		if (id.second.type_index == type)
		{
			result++;
		}
	}

	return result;
}


void fxm::clear()
{
	std::lock_guard<shared_mutex> lock(g_mutex);

	// Call recorded finalization functions for all IDs
	for (auto& id : fxm::map_t(std::move(g_map)))
	{
		if (id.second) (*id.first)(id.second.get());
	}
}

bool fxm::check(id_type_index_t type)
{
	reader_lock lock(g_mutex);

	const auto found = g_map.find(type);

	return found != g_map.end() && found->second;
}

std::shared_ptr<void> fxm::get(id_type_index_t type)
{
	reader_lock lock(g_mutex);

	const auto found = g_map.find(type);

	return found != g_map.end() ? found->second : nullptr;
}

std::shared_ptr<void> fxm::withdraw(id_type_index_t type)
{
	std::unique_lock<shared_mutex> lock(g_mutex);

	const auto found = g_map.find(type);

	return found != g_map.end() ? std::move(found->second) : nullptr;
}
