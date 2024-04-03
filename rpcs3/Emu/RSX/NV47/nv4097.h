// NV47 3D Engine
#pragma once

#include "common.h"

namespace rsx
{
	enum command_barrier_type;
	enum vertex_base_type;

	namespace nv4097
	{
		void clear(context* ctx, u32 reg, u32 arg);

		void clear_zcull(context* ctx, u32 reg, u32 arg);

		void set_face_property(context* ctx, u32 reg, u32 arg);

		void set_notify(context* ctx, u32 reg, u32 arg);

		void texture_read_semaphore_release(context* ctx, u32 reg, u32 arg);

		void back_end_write_semaphore_release(context* ctx, u32 reg, u32 arg);

		void set_array_element16(context* ctx, u32, u32 arg);

		void set_array_element32(context* ctx, u32, u32 arg);

		void draw_arrays(context* /*rsx*/, u32 reg, u32 arg);

		void draw_index_array(context* /*rsx*/, u32 reg, u32 arg);

		void draw_inline_array(context* /*rsx*/, u32 reg, u32 arg);

		void set_transform_program_start(context* ctx, u32 reg, u32);

		void set_vertex_attribute_output_mask(context* ctx, u32 reg, u32);

		void set_begin_end(context* ctxthr, u32 reg, u32 arg);

		void get_report(context* ctx, u32 reg, u32 arg);

		void clear_report_value(context* ctx, u32 reg, u32 arg);

		void set_render_mode(context* ctx, u32, u32 arg);

		void set_zcull_render_enable(context* ctx, u32, u32);

		void set_zcull_stats_enable(context* ctx, u32, u32);

		void set_zcull_pixel_count_enable(context* ctx, u32, u32);

		void sync(context* ctx, u32, u32);

		void set_shader_program_dirty(context* ctx, u32, u32);

		void set_surface_dirty_bit(context* ctx, u32 reg, u32 arg);

		void set_surface_format(context* ctx, u32 reg, u32 arg);

		void set_surface_options_dirty_bit(context* ctx, u32 reg, u32 arg);

		void set_color_mask(context* ctx, u32 reg, u32 arg);

		void set_stencil_op(context* ctx, u32 reg, u32 arg);

		void set_vertex_base_offset(context* ctx, u32 reg, u32 arg);

		void set_index_base_offset(context* ctx, u32 reg, u32 arg);

		void check_index_array_dma(context* ctx, u32 reg, u32 arg);

		void set_blend_equation(context* ctx, u32 reg, u32 arg);

		void set_blend_factor(context* ctx, u32 reg, u32 arg);

#define RSX(ctx) ctx->rsxthr
#define REGS(ctx) (&rsx::method_registers)

		/**
		* id = base method register
		* index = register index in method
		* count = element count per attribute
		* register_count = number of registers consumed per attribute. E.g 3-element methods have padding
		*/
		template<u32 id, u32 index, int count, int register_count, typename type>
		void set_vertex_data_impl(context* ctx, u32 arg)
		{
			static constexpr usz increment_per_array_index = (register_count * sizeof(type)) / sizeof(u32);

			static constexpr usz attribute_index = index / increment_per_array_index;
			static constexpr usz vertex_subreg = index % increment_per_array_index;

			constexpr auto vtype = vertex_data_type_from_element_type<type>::type;
			static_assert(vtype != rsx::vertex_base_type::cmp);
			static_assert(vtype != rsx::vertex_base_type::ub256);

			// Convert LE data to BE layout
			if constexpr (sizeof(type) == 4)
			{
				arg = std::bit_cast<u32, be_t<u32>>(arg);
			}
			else if constexpr (sizeof(type) == 2)
			{
				// 2 16-bit values packed in 1 32-bit word
				const auto be_data = std::bit_cast<u32, be_t<u32>>(arg);

				// After u32 swap, the components are in the wrong position
				arg = (be_data << 16) | (be_data >> 16);
			}

			util::push_vertex_data(attribute_index, vertex_subreg, count, vtype);
		}

		template<u32 index>
		struct set_vertex_data4ub_m
		{
			static void impl(context* ctx, u32 reg, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA4UB_M, index, 4, 4, u8>(ctx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data1f_m
		{
			static void impl(context* ctx, u32 reg, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA1F_M, index, 1, 1, f32>(ctx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data2f_m
		{
			static void impl(context* ctx, u32 reg, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA2F_M, index, 2, 2, f32>(ctx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data3f_m
		{
			static void impl(context* ctx, u32 reg, u32 arg)
			{
				//Register alignment is only 1, 2, or 4 (Rachet & Clank 2)
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA3F_M, index, 3, 4, f32>(ctx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data4f_m
		{
			static void impl(context* ctx, u32 reg, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA4F_M, index, 4, 4, f32>(ctx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data2s_m
		{
			static void impl(context* ctx, u32 reg, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA2S_M, index, 2, 2, u16>(ctx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data4s_m
		{
			static void impl(context* ctx, u32 reg, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA4S_M, index, 4, 4, u16>(ctx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data_scaled4s_m
		{
			static void impl(context* ctx, u32 reg, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA_SCALED4S_M, index, 4, 4, s16>(ctx, arg);
			}
		};

		struct set_transform_constant
		{
			static void impl(context* ctx, u32 reg, u32 arg);
		};

		struct set_transform_program
		{
			static void impl(context* ctx, u32 reg, u32 arg);
		};

		template<u32 index>
		struct set_vertex_array_offset
		{
			static void impl(context* ctx, u32 reg, u32 arg)
			{
				util::push_draw_parameter_change(ctx, vertex_array_offset_modifier_barrier, reg, arg);
			}
		};

		template<u32 index>
		struct set_texture_dirty_bit
		{
			static void impl(context* ctx, u32 reg, u32 arg)
			{
				RSX(ctx)->m_textures_dirty[index] = true;

				if (RSX(ctx)->current_fp_metadata.referenced_textures_mask & (1 << index))
				{
					RSX(ctx)->m_graphics_state |= rsx::pipeline_state::fragment_program_state_dirty;
				}
			}
		};

		template<u32 index>
		struct set_vertex_texture_dirty_bit
		{
			static void impl(context* ctx, u32 reg, u32 arg)
			{
				RSX(ctx)->m_vertex_textures_dirty[index] = true;

				if (RSX(ctx)->current_vp_metadata.referenced_textures_mask & (1 << index))
				{
					RSX(ctx)->m_graphics_state |= rsx::pipeline_state::vertex_program_state_dirty;
				}
			}
		};

#undef RSX
#undef REGS
	}
}
