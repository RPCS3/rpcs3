#pragma once

#include "draw_call.inc.h"

#include "Emu/RSX/Common/simple_array.hpp"
#include "Emu/RSX/gcm_enums.h"

namespace rsx
{
	class draw_clause
	{
		// Stores the first and count argument from draw/draw indexed parameters between begin/end clauses.
		simple_array<draw_range_t> draw_command_ranges{};

		// Stores rasterization barriers for primitive types sensitive to adjacency
		simple_array<barrier_t> draw_command_barriers{};

		// Counter used to parse the commands in order
		u32 current_range_index{};

		// Location of last execution barrier
		u32 last_execution_barrier_index{};

		// Helper functions
		// Add a new draw command
		void append_draw_command(const draw_range_t& range)
		{
			current_range_index = draw_command_ranges.size();
			draw_command_ranges.push_back(range);
		}

		// Insert a new draw command within the others
		void insert_draw_command(u32 index, const draw_range_t& range)
		{
			auto range_It = draw_command_ranges.begin();
			std::advance(range_It, index);

			current_range_index = index;
			draw_command_ranges.insert(range_It, range);

			// Update all barrier draw ids after this one
			for (auto& barrier : draw_command_barriers)
			{
				if (barrier.draw_id >= index)
				{
					barrier.draw_id++;
				}
			}
		}

	public:
		primitive_type primitive{};
		draw_command command{};

		bool is_immediate_draw{};          // Set if part of the draw is submitted via push registers
		bool is_disjoint_primitive{};      // Set if primitive type does not rely on adjacency information
		bool primitive_barrier_enable{};   // Set once to signal that a primitive restart barrier can be inserted

		simple_array<u32> inline_vertex_array{};

		void operator()(utils::serial& ar);

		void insert_command_barrier(command_barrier_type type, u32 arg, u32 register_index = 0);

		/**
		 * Optimize commands for rendering
		 */
		void compile()
		{
			// End draw call append mode
			current_range_index = ~0u;

			// TODO
		}

		/**
		 * Insert one command range
		 */

		void append(u32 first, u32 count)
		{
			const bool barrier_enable_flag = primitive_barrier_enable;
			primitive_barrier_enable = false;

			if (!draw_command_ranges.empty())
			{
				auto& last = draw_command_ranges[current_range_index];

				if (last.count == 0)
				{
					// Special case, usually indicates an execution barrier
					last.first = first;
					last.count = count;
					return;
				}

				if (last.first + last.count == first)
				{
					if (!is_disjoint_primitive && barrier_enable_flag)
					{
						// Insert barrier
						insert_command_barrier(primitive_restart_barrier, 0);
					}

					last.count += count;
					return;
				}

				for (auto index = last_execution_barrier_index; index < draw_command_ranges.size(); ++index)
				{
					if (draw_command_ranges[index].first == first &&
						draw_command_ranges[index].count == count)
					{
						// Duplicate entry? WTF!
						return;
					}

					if (draw_command_ranges[index].first > first)
					{
						insert_draw_command(index, { 0, first, count });
						return;
					}
				}
			}

			append_draw_command({ 0, first, count });
		}

		/**
		 * Returns how many vertex or index will be consumed by the draw clause.
		 */
		u32 get_elements_count() const
		{
			if (draw_command_ranges.empty())
			{
				ensure(command == rsx::draw_command::inlined_array);
				return 0;
			}

			return get_range().count;
		}

		u32 min_index() const
		{
			if (draw_command_ranges.empty())
			{
				ensure(command == rsx::draw_command::inlined_array);
				return 0;
			}

			return get_range().first;
		}

		bool is_single_draw() const
		{
			if (is_disjoint_primitive)
				return true;

			if (draw_command_ranges.empty())
			{
				ensure(!inline_vertex_array.empty());
				return true;
			}

			ensure(current_range_index != ~0u);
			for (const auto& barrier : draw_command_barriers)
			{
				if (barrier.draw_id != current_range_index)
					continue;

				if (barrier.type == primitive_restart_barrier)
					return false;
			}

			return true;
		}

		bool empty() const
		{
			return (command == rsx::draw_command::inlined_array) ? inline_vertex_array.empty() : draw_command_ranges.empty();
		}

		u32 pass_count() const
		{
			if (draw_command_ranges.empty())
			{
				ensure(!inline_vertex_array.empty());
				return 1u;
			}

			u32 count = ::size32(draw_command_ranges);
			if (draw_command_ranges.back().count == 0)
			{
				// Dangling barrier
				ensure(count > 1);
				count--;
			}

			return count;
		}

		primitive_class classify_mode() const
		{
			return primitive >= rsx::primitive_type::triangles
				? primitive_class::polygon
				: primitive_class::non_polygon;
		}


		void reset(rsx::primitive_type type);

		void begin()
		{
			current_range_index = 0;
		}

		void end()
		{
			current_range_index = draw_command_ranges.size() - 1;
		}

		bool next()
		{
			current_range_index++;
			if (current_range_index >= draw_command_ranges.size())
			{
				current_range_index = 0;
				return false;
			}

			if (draw_command_ranges[current_range_index].count == 0)
			{
				// Dangling execution barrier
				ensure(current_range_index > 0 && (current_range_index + 1) == draw_command_ranges.size());
				current_range_index = 0;
				return false;
			}

			return true;
		}

		/**
		 * Only call this once after the draw clause has been fully consumed to reconcile any conflicts
		 */
		void post_execute_cleanup(struct context* ctx)
		{
			ensure(current_range_index == 0);

			if (draw_command_ranges.size() > 1)
			{
				if (draw_command_ranges.back().count == 0)
				{
					// Dangling execution barrier
					current_range_index = draw_command_ranges.size() - 1;
					execute_pipeline_dependencies(ctx);
					current_range_index = 0;
				}
			}
		}

		/**
		 * Executes commands reqiured to make the current draw state valid
		 */
		u32 execute_pipeline_dependencies(struct context* ctx) const;

		const draw_range_t& get_range() const
		{
			ensure(current_range_index < draw_command_ranges.size());
			return draw_command_ranges[current_range_index];
		}

		simple_array<draw_range_t> get_subranges() const
		{
			ensure(!is_single_draw());

			const auto range = get_range();
			const auto limit = range.first + range.count;

			simple_array<draw_range_t> ret;
			u32 previous_barrier = range.first;
			u32 vertex_counter = 0;

			for (const auto& barrier : draw_command_barriers)
			{
				if (barrier.draw_id != current_range_index)
					continue;

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
	};
}
