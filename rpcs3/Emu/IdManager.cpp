#include "stdafx.h"
#include "IdManager.h"

shared_mutex id_manager::g_mutex;

thread_local DECLARE(idm::g_id);
DECLARE(idm::g_map);
DECLARE(fxm::g_vec);

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

id_manager::id_map::pointer idm::allocate_id(std::pair<u32, u32> types, u32 base, u32 step, u32 count)
{
	auto& map = g_map[types.first];

	// Assume next ID
	u32 next = base;
	
	if (std::size_t _count = map.size())
	{
		if (_count >= count)
		{
			return nullptr;
		}

		const u32 _next = map.rbegin()->first.id() + step;

		if (_next > base && _next < base + step * count)
		{
			next = _next;
		}
	}

	// Check all IDs starting from "next id" (TODO)
	for (next; next < base + step * count; next += step)
	{
		// Get ID
		const auto result = map.emplace(id_manager::id_key(next, types.second), nullptr);

		if (result.second)
		{
			// Acknowledge the ID
			g_id = next;

			return std::addressof(*result.first);
		}
	}

	// Nothing found
	return nullptr;
}

id_manager::id_map::pointer idm::find_id(u32 type, u32 id)
{
	auto& map = g_map[type];

	const auto found = map.find(id);

	return found == map.end() ? nullptr : std::addressof(*found);
}

id_manager::id_map::pointer idm::find_id(u32 type, u32 true_type, u32 id)
{
	auto& map = g_map[type];

	const auto found = map.find(id);

	return found == map.end() || found->first.type() != true_type ? nullptr : std::addressof(*found);
}

std::shared_ptr<void> idm::delete_id(u32 type, u32 id)
{
	auto& map = g_map[type];

	const auto found = map.find(id);

	std::shared_ptr<void> result;

	if (found != map.end())
	{
		result = std::move(found->second);
		map.erase(found);
	}

	return result;
}

std::shared_ptr<void> idm::delete_id(u32 type, u32 true_type, u32 id)
{
	auto& map = g_map[type];

	const auto found = map.find(id);

	std::shared_ptr<void> result;

	if (found != map.end() && found->first.type() == true_type)
	{
		result = std::move(found->second);
		map.erase(found);
	}

	return result;
}

void idm::init()
{
	g_map.resize(id_manager::typeinfo::get().size());
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
	}
}

void fxm::init()
{
	g_vec.resize(id_manager::typeinfo::get().size(), {});
}

void fxm::clear()
{
	// Call recorded finalization functions for all IDs
	for (std::size_t i = 0; i < g_vec.size(); i++)
	{
		if (g_vec[i])
		{
			id_manager::typeinfo::get()[i].on_stop(g_vec[i].get());
		}

		g_vec[i].reset();
	}
}
