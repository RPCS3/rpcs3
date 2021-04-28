#include "stdafx.h"
#include "IdManager.h"
#include "Utilities/Thread.h"

shared_mutex id_manager::g_mutex;

thread_local DECLARE(idm::g_id);

idm::map_data* idm::allocate_id(std::vector<map_data>& vec, u32 type_id,  u32 base, u32 step, u32 count, std::pair<u32, u32> invl_range)
{
	if (vec.size() < count)
	{
		// Try to emplace back
		const u32 _next = base + step * ::size32(vec);
		g_id = _next;
		vec.emplace_back(id_manager::id_key(_next, type_id), nullptr);
		return &vec.back();
	}

	// Check all IDs starting from "next id" (TODO)
	for (u32 i = 0, next = base; i < count; i++, next += step)
	{
		const auto ptr = &vec[i];

		// Look for free ID
		if (!ptr->second)
		{
			// Incremenet ID invalidation counter
			const u32 id = next | ((ptr->first + (1u << invl_range.first)) & (invl_range.second ? (((1u << invl_range.second) - 1) << invl_range.first) : 0));
			g_id = id;
			ptr->first = id_manager::id_key(id, type_id);
			return ptr;
		}
	}

	// Out of IDs
	return nullptr;
}
