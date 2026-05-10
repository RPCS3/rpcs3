#pragma once

#include <util/types.hpp>
#include "context.h"
#include "context_accessors.define.h"

namespace rsx
{
	enum command_barrier_type : u32;
	enum class vertex_base_type : u8;

	namespace util
	{
		u32 get_report_data_impl(rsx::context* ctx, u32 offset);

		void push_vertex_data(rsx::context* ctx, u32 attrib_index, u32 channel_select, int count, rsx::vertex_base_type vtype, u32 value);

		void push_draw_parameter_change(rsx::context* ctx, rsx::command_barrier_type type, u32 reg, u32 arg0, u32 arg1 = 0u, u32 index = 0u);

		void set_fragment_texture_dirty_bit(rsx::context* ctx, u32 arg, u32 index);

		void set_vertex_texture_dirty_bit(rsx::context* ctx, u32 index);
	}
}
 
#include "context_accessors.undef.h"
