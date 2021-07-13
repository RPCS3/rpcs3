#include "stdafx.h"
#include "IdManager.h"

#include "Utilities/Thread.h"

shared_mutex id_manager::g_mutex;

namespace id_manager
{
	thread_local u32 g_id = 0;
}

idm::map_data* idm::allocate_id(std::vector<map_data>& vec, u64 type_id, u32 dst_id, u32 base, u32 step, u32 count, std::pair<u32, u32> invl_range)
{
	if (const u32 index = id_manager::get_index(dst_id, base, step, count, invl_range); index < count)
	{
		// Fixed position construction
		ensure(index < vec.size());

		if (vec[index].second)
		{
			return nullptr;
		}

		id_manager::g_id = dst_id;
		vec[index] = {id_manager::id_key(dst_id, type_id), nullptr};
		return &vec[index];
	}

	if (vec.size() < count)
	{
		// Try to emplace back
		const u32 _next = base + step * ::size32(vec);
		id_manager::g_id = _next;
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
			id_manager::g_id = id;
			ptr->first = id_manager::id_key(id, type_id);
			return ptr;
		}
	}

	// Out of IDs
	return nullptr;
}
