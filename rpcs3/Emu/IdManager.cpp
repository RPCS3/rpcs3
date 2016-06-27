#include "stdafx.h"
#include "IdManager.h"

DECLARE(idm::g_map);
DECLARE(idm::g_id);
DECLARE(idm::g_mutex);

DECLARE(fxm::g_map);
DECLARE(fxm::g_mutex);

std::vector<id_manager::typeinfo>& id_manager::typeinfo::access()
{
	static std::vector<typeinfo> list;
	
	return list;
}

u32 id_manager::typeinfo::add_type()
{
	auto& list = access();

	list.emplace_back();

	return ::size32(list) - 1;
}

idm::map_type::pointer idm::allocate_id(u32 tag, u32 min, u32 max)
{
	// Check all IDs starting from "next id"
	for (u32 i = 0; i <= max - min; i++)
	{
		// Fix current ID (wrap around)
		if (g_id[tag] < min || g_id[tag] > max) g_id[tag] = min;

		// Get ID
		const auto r = g_map[tag].emplace(g_id[tag]++, nullptr);

		if (r.second)
		{
			return &*r.first;
		}
	}

	// Nothing found
	return nullptr;
}

std::shared_ptr<void> idm::deallocate_id(u32 tag, u32 id)
{
	const auto found = g_map[tag].find(id);

	if (found == g_map[tag].end()) return nullptr;

	auto ptr = std::move(found->second);

	g_map[tag].erase(found);

	return ptr;
}

idm::map_type::pointer idm::find_id(u32 type, u32 id)
{
	const auto found = g_map[type].find(id);

	if (found == g_map[type].end()) return nullptr;

	return &*found;
}

std::shared_ptr<void> idm::delete_id(u32 type, u32 tag, u32 id)
{
	writer_lock lock(g_mutex);

	auto&& ptr = deallocate_id(tag, id);

	g_map[type].erase(id);

	return ptr;
}

void idm::init()
{
	g_map.resize(id_manager::typeinfo::get().size(), {});
	g_id.resize(id_manager::typeinfo::get().size(), 0);
}

void idm::clear()
{
	// Call recorded finalization functions for all IDs
	for (std::size_t i = 0; i < g_map.size(); i++)
	{
		const auto on_stop = id_manager::typeinfo::get()[i].on_stop;

		for (auto& id : g_map[i])
		{
			on_stop(id.second.get());
		}

		g_map[i].clear();
		g_id[i] = 0;
	}
}

std::shared_ptr<void> fxm::remove(u32 type)
{
	writer_lock lock(g_mutex);

	return std::move(g_map[type]);
}

void fxm::init()
{
	g_map.resize(id_manager::typeinfo::get().size(), {});
}

void fxm::clear()
{
	// Call recorded finalization functions for all IDs
	for (std::size_t i = 0; i < g_map.size(); i++)
	{
		if (g_map[i])
		{
			id_manager::typeinfo::get()[i].on_stop(g_map[i].get());
		}

		g_map[i].reset();
	}
}
