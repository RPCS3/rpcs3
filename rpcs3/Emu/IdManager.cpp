#include "stdafx.h"
#include "IdManager.h"

shared_mutex id_manager::g_mutex;

thread_local DECLARE(idm::g_id);
DECLARE(idm::g_map);
DECLARE(fxm::g_vec);

id_manager::id_map::pointer idm::allocate_id(const id_manager::id_key& info, u32 base, u32 step, u32 count)
{
	// Base type id is stored in value
	auto& vec = g_map[info.value()];

	// Preallocate memory
	vec.reserve(count);

	if (vec.size() < count)
	{
		// Try to emplace back
		const u32 _next = base + step * ::size32(vec);

		if (_next >= base && _next < base + step * count)
		{
			g_id = _next;
			vec.emplace_back(id_manager::id_key(_next, info.type(), info.on_stop()), nullptr);
			return &vec.back();
		}
	}

	// Check all IDs starting from "next id" (TODO)
	for (u32 i = 0, next = base; i < count; i++, next += step)
	{
		const auto ptr = &vec[i];

		// Look for free ID
		if (!ptr->second)
		{
			g_id = next;
			ptr->first = id_manager::id_key(next, info.type(), info.on_stop());
			return ptr;
		}
	}

	// Out of IDs
	return nullptr;
}

void idm::init()
{
	// Allocate
	g_map.resize(id_manager::typeinfo::get_count());
	idm::clear();
}

void idm::clear()
{
	// Call recorded finalization functions for all IDs
	for (auto& map : g_map)
	{
		for (auto& pair : map)
		{
			if (auto ptr = pair.second.get())
			{
				pair.first.on_stop()(ptr);
				pair.second.reset();
				pair.first = {};
			}
		}

		map.clear();
	}
}

void fxm::init()
{
	// Allocate
	g_vec.resize(id_manager::typeinfo::get_count());
	fxm::clear();
}

void fxm::clear()
{
	// Call recorded finalization functions for all IDs
	for (auto& pair : g_vec)
	{
		if (auto ptr = pair.second.get())
		{
			pair.first(ptr);
			pair.second.reset();
			pair.first = nullptr;
		}
	}
}
