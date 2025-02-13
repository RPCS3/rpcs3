#include "stdafx.h"
#include "IdManager.h"

shared_mutex id_manager::g_mutex;

namespace id_manager
{
	thread_local u32 g_id = 0;
}

template <>
bool serialize<std::shared_ptr<utils::serial>>(utils::serial& ar, std::shared_ptr<utils::serial>& o)
{
	if (!o)
	{
		o = std::make_shared<utils::serial>();
	}

	if (!ar.is_writing())
	{
		o->set_reading_state();
	}

	ar(o->data);
	return true;
}

std::vector<std::pair<u128, id_manager::typeinfo>>& id_manager::get_typeinfo_map()
{
	// Magic static
	static std::vector<std::pair<u128, id_manager::typeinfo>> s_map;
	return s_map;
}

id_manager::id_key* idm::allocate_id(std::span<id_manager::id_key> keys, u32& highest_index, u32 type_id, u32 dst_id, u32 base, u32 step, u32 count, bool uses_lowest_id, std::pair<u32, u32> invl_range)
{
	if (dst_id != (base ? 0 : u32{umax}))
	{
		// Fixed position construction
		const u32 index = id_manager::get_index(dst_id, base, step, count, invl_range);
		ensure(index < count);

		highest_index = std::max(highest_index, index + 1);

		if (keys[index].type() != umax)
		{
			return nullptr;
		}

		id_manager::g_id = dst_id;
		keys[index] = id_manager::id_key(dst_id, type_id);
		return &keys[index];
	}

	if (uses_lowest_id)
	{
		// Disable the optimization below (hurts accuracy for known cases)
		highest_index = count;
	}
	else if (highest_index < count)
	{
		// Try to emplace back
		const u32 _next = base + step * highest_index;
		id_manager::g_id = _next;
		return &(keys[highest_index++] = (id_manager::id_key(_next, type_id)));
	}

	// Check all IDs starting from "next id" (TODO)
	for (u32 i = 0, next = base; i < count; i++, next += step)
	{
		const auto ptr = &keys[i];

		// Look for free ID
		if (ptr->type() == umax)
		{
			// Incremenet ID invalidation counter
			const u32 id = next | ((ptr->value() + (1u << invl_range.first)) & (invl_range.second ? (((1u << invl_range.second) - 1) << invl_range.first) : 0));
			id_manager::g_id = id;
			*ptr = id_manager::id_key(id, type_id);
			return ptr;
		}
	}

	// Out of IDs
	return nullptr;
}
