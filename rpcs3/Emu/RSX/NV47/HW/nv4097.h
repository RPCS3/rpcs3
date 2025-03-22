// NV47 3D Engine
#pragma once

#include "common.h"

#include "Emu/RSX/gcm_enums.h"
#include "Emu/RSX/NV47/FW/draw_call.inc.h"

#include <span>

namespace rsx
{
	enum command_barrier_type : u32;

	namespace nv4097
	{
		template<typename Type> struct vertex_data_type_from_element_type;
		template<> struct vertex_data_type_from_element_type<float> { static const vertex_base_type type = vertex_base_type::f; };
		template<> struct vertex_data_type_from_element_type<f16> { static const vertex_base_type type = vertex_base_type::sf; };
		template<> struct vertex_data_type_from_element_type<u8> { static const vertex_base_type type = vertex_base_type::ub; };
		template<> struct vertex_data_type_from_element_type<u16> { static const vertex_base_type type = vertex_base_type::s32k; };
		template<> struct vertex_data_type_from_element_type<s16> { static const vertex_base_type type = vertex_base_type::s1; };

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

		void set_transform_constant_load(context* ctx, u32 reg, u32 arg);

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

			util::push_vertex_data(ctx, attribute_index, vertex_subreg, count, vtype, arg);
		}

		template<u32 index>
		struct set_vertex_data4ub_m
		{
			static void impl(context* ctx, u32 /*reg*/, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA4UB_M, index, 4, 4, u8>(ctx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data1f_m
		{
			static void impl(context* ctx, u32 /*reg*/, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA1F_M, index, 1, 1, f32>(ctx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data2f_m
		{
			static void impl(context* ctx, u32 /*reg*/, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA2F_M, index, 2, 2, f32>(ctx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data3f_m
		{
			static void impl(context* ctx, u32 /*reg*/, u32 arg)
			{
				//Register alignment is only 1, 2, or 4 (Rachet & Clank 2)
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA3F_M, index, 3, 4, f32>(ctx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data4f_m
		{
			static void impl(context* ctx, u32 /*reg*/, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA4F_M, index, 4, 4, f32>(ctx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data2s_m
		{
			static void impl(context* ctx, u32 /*reg*/, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA2S_M, index, 2, 2, u16>(ctx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data4s_m
		{
			static void impl(context* ctx, u32 /*reg*/, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA4S_M, index, 4, 4, u16>(ctx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data_scaled4s_m
		{
			static void impl(context* ctx, u32 /*reg*/, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA_SCALED4S_M, index, 4, 4, s16>(ctx, arg);
			}
		};

		struct set_transform_constant
		{
			static void impl(context* ctx, u32 reg, u32 arg);

			static void decode_one(context* ctx, u32 reg, u32 arg);

			static void batch_decode(context* ctx, u32 reg, const std::span<const u32>& args, const std::function<bool(context*, u32, u32)>& notify = {});
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
				util::push_draw_parameter_change(ctx, vertex_array_offset_modifier_barrier, reg, arg, 0, index);
			}
		};

		template <u32 index>
		struct set_fragment_texture_dirty_bit
		{
			static void impl(context* ctx, u32 /*reg*/, u32 arg)
			{
				util::set_fragment_texture_dirty_bit(ctx, arg, index);
			}
		};

		template<u32 index>
		struct set_vertex_texture_dirty_bit
		{
			static void impl(context* ctx, u32 /*reg*/, u32 /*arg*/)
			{
				util::set_vertex_texture_dirty_bit(ctx, index);
			}
		};

#undef RSX
#undef REGS
	}
}
