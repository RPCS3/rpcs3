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

	bool draw_clause::check_trivially_instanced() const
	{
		if (pass_count() <= 1)
		{
			// Cannot instance one draw call or less
			return false;
		}

		// For instancing all draw calls must be identical
		const auto& ref = draw_command_ranges.front();
		for (const auto& range : draw_command_ranges)
		{
			if (range.first != ref.first || range.count != ref.count)
			{
				return false;
			}
		}

		if (draw_command_barriers.empty())
		{
			// Raise alarm here for investigation, we may be missing a corner case.
			rsx_log.error("Instanced draw detected, but no command barriers found!");
			return false;
		}

		// Barriers must exist, but can only involve updating transform constants (for now)
		for (const auto& barrier : draw_command_barriers)
		{
			if (barrier.type != rsx::transform_constant_load_modifier_barrier &&
				barrier.type != rsx::transform_constant_update_barrier)
			{
				ensure(barrier.draw_id < ::size32(draw_command_ranges));
				if (draw_command_ranges[barrier.draw_id].count == 0)
				{
					// Dangling command barriers are ignored. We're also at the end of the command, so abort.
					break;
				}

				// Fail. Only transform constant instancing is supported at the moment.
				return false;
			}
		}

		return true;
	}

	void draw_clause::reset(primitive_type type)
	{
		current_range_index = ~0u;
		last_execution_barrier_index = 0;

		command = draw_command::none;
		primitive = type;
		primitive_barrier_enable = false;
		is_trivial_instanced_draw = false;

		draw_command_ranges.clear();
		draw_command_barriers.clear();
		inline_vertex_array.clear();

		is_disjoint_primitive = is_primitive_disjointed(primitive);
	}

	u32 draw_clause::execute_pipeline_dependencies(context* ctx, instanced_draw_config_t* instance_config) const
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

