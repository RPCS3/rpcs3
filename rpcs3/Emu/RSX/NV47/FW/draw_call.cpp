#include "stdafx.h"
#include "draw_call.hpp"

#include "Emu/RSX/rsx_methods.h" // FIXME
#include "Emu/RSX/rsx_utils.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/Common/BufferUtils.h"
#include "Emu/RSX/NV47/HW/context.h"
#include "Emu/RSX/NV47/HW/nv4097.h"

// Always import this after other HW definitions
#include "Emu/RSX/NV47/HW/context_accessors.define.h"

#include <util/serialization.hpp>

namespace rsx
{
	void draw_clause::operator()(utils::serial& ar)
	{
		ar(draw_command_ranges, draw_command_barriers, current_range_index, primitive, command, is_immediate_draw, is_disjoint_primitive, primitive_barrier_enable, inline_vertex_array);
	}

	void draw_clause::insert_command_barrier(command_barrier_type type, u32 arg0, u32 arg1, u32 index)
	{
		ensure(!draw_command_ranges.empty());

		auto _do_barrier_insert = [this](barrier_t&& val)
		{
			if (draw_command_barriers.empty() || draw_command_barriers.back() < val)
			{
				draw_command_barriers.push_back(val);
				return;
			}

			for (auto it = draw_command_barriers.begin(); it != draw_command_barriers.end(); it++)
			{
				if (*it < val)
				{
					continue;
				}

				draw_command_barriers.insert(it, val);
				break;
			}
		};

		draw_command_barrier_mask |= (1u << type);

		if (type == primitive_restart_barrier)
		{
			// Rasterization flow barrier
			const auto& last = draw_command_ranges[current_range_index];
			const auto address = last.first + last.count;

			_do_barrier_insert({
				.draw_id = current_range_index,
				.timestamp = 0,
				.address = address,
				.index = index,
				.arg0 = arg0,
				.arg1 = arg1,
				.flags = 0,
				.type = type
			});
		}
		else
		{
			// Execution dependency barrier. Requires breaking the current draw call sequence and start another.
			if (draw_command_ranges.back().count > 0)
			{
				append_draw_command({});
			}
			else
			{
				// In case of back-to-back modifiers, do not add duplicates
				current_range_index = draw_command_ranges.size() - 1;
			}

			_do_barrier_insert({
				.draw_id = current_range_index,
				.timestamp = rsx::get_shared_tag(),
				.address = ~0u,
				.index = index,
				.arg0 = arg0,
				.arg1 = arg1,
				.flags = 0,
				.type = type
			});

			last_execution_barrier_index = current_range_index;
		}
	}

	bool draw_clause::check_trivially_instanced() const
	{
		if (pass_count() <= 1)
		{
			// Cannot instance one draw call or less
			return false;
		}

		if (draw_command_barriers.empty())
		{
			return false;
		}

		// Barriers must exist, but can only involve updating transform constants (for now)
		const u32 compatible_barrier_mask =
			(1u << rsx::transform_constant_load_modifier_barrier) |
			(1u << rsx::transform_constant_update_barrier);

		if (draw_command_barrier_mask & ~compatible_barrier_mask)
		{
			return false;
		}

		// For instancing all draw calls must be identical
		// FIXME: This requirement can be easily lifted by chunking contiguous chunks.
		const auto& ref = draw_command_ranges.front();
		return !draw_command_ranges.any(FN(x.first != ref.first || x.count != ref.count));
	}

	void draw_clause::reset(primitive_type type)
	{
		current_range_index = ~0u;
		last_execution_barrier_index = 0;
		draw_command_barrier_mask = 0;

		command = draw_command::none;
		primitive = type;
		primitive_barrier_enable = false;
		is_trivial_instanced_draw = false;

		draw_command_ranges.clear();
		draw_command_barriers.clear();
		inline_vertex_array.clear();

		is_disjoint_primitive = is_primitive_disjointed(primitive);
	}

	const simple_array<draw_range_t>& draw_clause::get_subranges() const
	{
		ensure(!is_single_draw());

		const auto range = get_range();
		const auto limit = range.first + range.count;
		const auto _pass_count = pass_count();

		auto &ret = subranges_store;
		ret.clear();
		ret.reserve(_pass_count);

		u32 previous_barrier = range.first;
		u32 vertex_counter = 0;

		for (auto it = current_barrier_it;
			it != draw_command_barriers.end() && it->draw_id == current_range_index;
			it++)
		{
			const auto& barrier = *it;
			if (barrier.type != primitive_restart_barrier)
				continue;

			if (barrier.address <= range.first)
				continue;

			if (barrier.address >= limit)
				break;

			const u32 count = barrier.address - previous_barrier;
			ret.push_back({ 0, vertex_counter, count });
			previous_barrier = barrier.address;
			vertex_counter += count;
		}

		ensure(!ret.empty());
		ensure(previous_barrier < limit);
		ret.push_back({ 0, vertex_counter, limit - previous_barrier });

		return ret;
	}

	u32 draw_clause::execute_pipeline_dependencies(context* ctx, instanced_draw_config_t* instance_config) const
	{
		u32 result = 0u;
		for (auto it = current_barrier_it;
			it != draw_command_barriers.end() && it->draw_id == current_range_index;
			it++)
		{
			const auto& barrier = *it;
			switch (barrier.type)
			{
			case primitive_restart_barrier:
			{
				break;
			}
			case index_base_modifier_barrier:
			{
				// Change index base offset
				REGS(ctx)->decode(NV4097_SET_VERTEX_DATA_BASE_INDEX, barrier.arg0);
				result |= index_base_changed;
				break;
			}
			case vertex_base_modifier_barrier:
			{
				// Change vertex base offset
				REGS(ctx)->decode(NV4097_SET_VERTEX_DATA_BASE_OFFSET, barrier.arg0);
				result |= vertex_base_changed;
				break;
			}
			case vertex_array_offset_modifier_barrier:
			{
				// Change vertex array offset
				REGS(ctx)->decode(NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + barrier.index, barrier.arg0);
				result |= vertex_arrays_changed;
				break;
			}
			case transform_constant_load_modifier_barrier:
			{
				// Change the transform load target. Does not change result mask.
				REGS(ctx)->decode(NV4097_SET_TRANSFORM_PROGRAM_LOAD, barrier.arg0);
				break;
			}
			case transform_constant_update_barrier:
			{
				// Update transform constants
				auto ptr = RSX(ctx)->fifo_ctrl->translate_address(barrier.arg0);
				auto buffer = std::span<const u32>(static_cast<const u32*>(vm::base(ptr)), barrier.arg1);
				auto notify = [&](rsx::context*, u32 load, u32 count)
				{
					if (!instance_config)
					{
						return false;
					}

					instance_config->transform_constants_data_changed = true;
					instance_config->patch_load_offset = load;
					instance_config->patch_load_count = count;
					return true;
				};

				nv4097::set_transform_constant::batch_decode(ctx, NV4097_SET_TRANSFORM_CONSTANT + barrier.index, buffer, notify);
				result |= transform_constants_changed;
				break;
			}
			default:
				fmt::throw_exception("Unreachable");
			}
		}

		return result;
	}
}

