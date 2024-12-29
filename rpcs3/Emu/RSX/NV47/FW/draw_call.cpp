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

	void draw_clause::reset(primitive_type type)
	{
		current_range_index = ~0u;
		last_execution_barrier_index = 0;

		command = draw_command::none;
		primitive = type;
		primitive_barrier_enable = false;

		draw_command_ranges.clear();
		draw_command_barriers.clear();
		inline_vertex_array.clear();

		is_disjoint_primitive = is_primitive_disjointed(primitive);
	}

	u32 draw_clause::execute_pipeline_dependencies(context* ctx) const
	{
		u32 result = 0u;
		for (;
			current_barrier_it != draw_command_barriers.end() && current_barrier_it->draw_id == current_range_index;
			current_barrier_it++)
		{
			const auto& barrier = *current_barrier_it;
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
				nv4097::set_transform_constant::batch_decode(ctx, NV4097_SET_TRANSFORM_CONSTANT + barrier.index, buffer);
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

