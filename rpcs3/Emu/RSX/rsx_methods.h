#pragma once

#include <array>
#include <vector>
#include <numeric>
#include <deque>
#include <set>

#include "GCM.h"
#include "rsx_decode.h"
#include "RSXTexture.h"
#include "rsx_vertex_data.h"
#include "rsx_utils.h"
#include "Utilities/geometry.h"

#include <cereal/types/array.hpp>
#include <cereal/types/unordered_map.hpp>

extern u64 get_system_time();
extern bool is_primitive_disjointed(rsx::primitive_type);

namespace rsx
{
	enum class draw_command
	{
		none,
		array,
		inlined_array,
		indexed,
	};

	enum command_barrier_type : u32
	{
		primitive_restart_barrier,
		vertex_base_modifier_barrier,
		index_base_modifier_barrier
	};

	enum command_execution_flags : u32
	{
		vertex_base_changed = (1 << 0),
		index_base_changed = (1 << 1)
	};

	struct barrier_t
	{
		u32 draw_id;
		u64 timestamp;

		u32 address;
		u32 arg;
		u32 flags;
		command_barrier_type type;

		bool operator < (const barrier_t& other) const
		{
			if (address != ~0u)
			{
				return address < other.address;
			}

			return timestamp < other.timestamp;
		}
	};

	struct draw_range_t
	{
		u32 command_data_offset = 0;
		u32 first = 0;
		u32 count = 0;
	};

	class draw_clause
	{
		// Stores the first and count argument from draw/draw indexed parameters between begin/end clauses.
		simple_array<draw_range_t> draw_command_ranges;

		// Stores rasterization barriers for primitive types sensitive to adjacency
		simple_array<barrier_t> draw_command_barriers;

		// Counter used to parse the commands in order
		u32 current_range_index;

		// Location of last execution barrier
		u32 last_execution_barrier_index;

		// Helper functions
		// Add a new draw command
		void append_draw_command(const draw_range_t& range)
		{
			draw_command_ranges.push_back(range);
		}

		// Insert a new draw command within the others
		void insert_draw_command(u32 index, const draw_range_t& range)
		{
			auto range_It = draw_command_ranges.begin();
			std::advance(range_It, index);

			draw_command_ranges.insert(range_It, range);

			// Update all barrier draw ids after this one
			for (auto &barrier : draw_command_barriers)
			{
				if (barrier.draw_id >= index)
				{
					barrier.draw_id++;
				}
			}
		}

	public:
		primitive_type primitive;
		draw_command command;

		bool is_immediate_draw;          // Set if part of the draw is submitted via push registers
		bool is_disjoint_primitive;      // Set if primitive type does not rely on adjacency information
		bool primitive_barrier_enable;   // Set once to signal that a primitive restart barrier can be inserted

		simple_array<u32> inline_vertex_array;

		void insert_command_barrier(command_barrier_type type, u32 arg)
		{
			verify(HERE), !draw_command_ranges.empty();

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
				const auto& last = draw_command_ranges.back();
				const auto address = last.first + last.count;

				const auto command_index = draw_command_ranges.size() - 1;
				_do_barrier_insert({ command_index, 0, address, arg, 0, type });
			}
			else
			{
				// Execution dependency barrier
				append_draw_command({});
				const auto command_index = draw_command_ranges.size() - 1;

				_do_barrier_insert({ command_index, get_system_time(), ~0u, arg, 0, type });
				last_execution_barrier_index = command_index;
			}
		}

		/**
		 * Optimize commands for rendering
		 */
		void compile()
		{
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
				auto& last = draw_command_ranges.back();

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
				verify(HERE), command == rsx::draw_command::inlined_array;
				return 0;
			}

			return get_range().count;
		}

		u32 min_index() const
		{
			if (draw_command_ranges.empty())
			{
				verify(HERE), command == rsx::draw_command::inlined_array;
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
				verify(HERE), !inline_vertex_array.empty();
				return true;
			}

			verify(HERE), current_range_index != ~0u;
			for (const auto &barrier : draw_command_barriers)
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
				verify(HERE), !inline_vertex_array.empty();
				return 1u;
			}

			u32 count = ::size32(draw_command_ranges);
			if (draw_command_ranges.back().count == 0)
			{
				// Dangling barrier
				verify(HERE), count > 1;
				count--;
			}

			return count;
		}

		void reset(rsx::primitive_type type)
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
				verify(HERE), current_range_index > 0 && (current_range_index + 1) == draw_command_ranges.size();
				current_range_index = 0;
				return false;
			}

			return true;
		}

		/**
		 * Only call this once after the draw clause has been fully consumed to reconcile any conflicts
		 */
		void post_execute_cleanup()
		{
			verify(HERE), current_range_index == 0;

			if (draw_command_ranges.size() > 1)
			{
				if (draw_command_ranges.back().count == 0)
				{
					// Dangling execution barrier
					current_range_index = draw_command_ranges.size() - 1;
					execute_pipeline_dependencies();
					current_range_index = 0;
				}
			}
		}

		/**
		 * Executes commands reqiured to make the current draw state valid
		 */
		u32 execute_pipeline_dependencies() const;

		const draw_range_t& get_range() const
		{
			verify(HERE), current_range_index < draw_command_ranges.size();
			return draw_command_ranges[current_range_index];
		}

		simple_array<draw_range_t> get_subranges() const
		{
			verify(HERE), !is_single_draw();

			const auto range = get_range();
			const auto limit = range.first + range.count;

			simple_array<draw_range_t> ret;
			u32 previous_barrier = range.first;
			u32 vertex_counter = 0;

			for (const auto &barrier : draw_command_barriers)
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

			verify(HERE), !ret.empty(), previous_barrier < limit;
			ret.push_back({ 0, vertex_counter, limit - previous_barrier });

			return ret;
		}
	};

	using rsx_method_t = void(*)(class thread*, u32 reg, u32 arg);

	//TODO
	union alignas(4) method_registers_t
	{
		u8 _u8[0x10000];
		u32 _u32[0x10000 >> 2];
		/*
		struct alignas(4)
		{
		u8 pad[NV4097_SET_TEXTURE_OFFSET - 4];

		struct alignas(4) texture_t
		{
		u32 offset;

		union format_t
		{
		u32 _u32;

		struct
		{
		u32: 1;
		u32 location : 1;
		u32 cubemap : 1;
		u32 border_type : 1;
		u32 dimension : 4;
		u32 format : 8;
		u32 mipmap : 16;
		};
		} format;

		union address_t
		{
		u32 _u32;

		struct
		{
		u32 wrap_s : 4;
		u32 aniso_bias : 4;
		u32 wrap_t : 4;
		u32 unsigned_remap : 4;
		u32 wrap_r : 4;
		u32 gamma : 4;
		u32 signed_remap : 4;
		u32 zfunc : 4;
		};
		} address;

		u32 control0;
		u32 control1;
		u32 filter;
		u32 image_rect;
		u32 border_color;
		} textures[limits::textures_count];
		};
		*/
		u32& operator[](int index)
		{
			return _u32[index >> 2];
		}
	};

	struct rsx_state
	{
	public:
		std::array<u32, 0x10000 / 4> registers;
		u32 register_previous_value;

		template<u32 opcode>
		using decoded_type = typename registers_decoder<opcode>::decoded_type;

		template<u32 opcode>
		decoded_type<opcode> decode() const
		{
			u32 register_value = registers[opcode];
			return decoded_type<opcode>(register_value);
		}

		rsx_state &operator=(const rsx_state& in)
		{
			registers = in.registers;
			transform_program = in.transform_program;
			transform_constants = in.transform_constants;
			register_vertex_info = in.register_vertex_info;
			return *this;
		}

		std::array<fragment_texture, 16> fragment_textures;
		std::array<vertex_texture, 4> vertex_textures;


		std::array<u32, 512 * 4> transform_program;
		std::array<u32[4], 512> transform_constants;

		draw_clause current_draw_clause;

		/**
		* RSX can sources vertex attributes from 2 places:
		* 1. Immediate values passed by NV4097_SET_VERTEX_DATA*_M + ARRAY_ID write.
		* For a given ARRAY_ID the last command of this type defines the actual type of the immediate value.
		* If there is only a single value on an ARRAY_ID passed this way, all vertex in the draw call
		* shares it.
		* Immediate mode rendering uses this method as well to upload vertex data.
		*
		* 2. Vertex array values passed by offset/stride/size/format description.
		* A given ARRAY_ID can have both an immediate value and a vertex array enabled at the same time
		* (See After Burner Climax intro cutscene). In such case the vertex array has precedence over the
		* immediate value. As soon as the vertex array is disabled (size set to 0) the immediate value
		* must be used if the vertex attrib mask request it.
		*
		* Note that behavior when both vertex array and immediate value system are disabled but vertex attrib mask
		* request inputs is unknown.
		*/
		std::array<register_vertex_data_info, 16> register_vertex_info;
		std::array<data_array_format_info, 16> vertex_arrays_info;

	private:
		template<typename T, size_t... N, typename Args>
		static std::array<T, sizeof...(N)> fill_array(Args&& arg, std::index_sequence<N...>)
		{
			return{ T(N, std::forward<Args>(arg))... };
		}

	public:
		rsx_state() :
			fragment_textures(fill_array<fragment_texture>(registers, std::make_index_sequence<16>())),
			vertex_textures(fill_array<vertex_texture>(registers, std::make_index_sequence<4>())),
			vertex_arrays_info(fill_array<data_array_format_info>(registers, std::make_index_sequence<16>()))
		{
			//NOTE: Transform constants persist through a context reset (NPEB00913)
			transform_constants = {};
		}

		~rsx_state() = default;

		void decode(u32 reg, u32 value);

		bool test(u32 reg, u32 value) const;

		void reset();

		void init();

		template<typename Archive>
		void serialize(Archive & ar)
		{
			ar(transform_program,
//				transform_constants,
				registers
				);
		}

		u16 viewport_width() const
		{
			return decode<NV4097_SET_VIEWPORT_HORIZONTAL>().width();
		}

		u16 viewport_origin_x() const
		{
			return decode<NV4097_SET_VIEWPORT_HORIZONTAL>().origin_x();
		}

		u16 viewport_height() const
		{
			return decode<NV4097_SET_VIEWPORT_VERTICAL>().height();
		}

		u16 viewport_origin_y() const
		{
			return decode<NV4097_SET_VIEWPORT_VERTICAL>().origin_y();
		}

		u16 scissor_origin_x() const
		{
			return decode<NV4097_SET_SCISSOR_HORIZONTAL>().origin_x();
		}

		u16 scissor_width() const
		{
			return decode<NV4097_SET_SCISSOR_HORIZONTAL>().width();
		}

		u16 scissor_origin_y() const
		{
			return decode<NV4097_SET_SCISSOR_VERTICAL>().origin_y();
		}

		u16 scissor_height() const
		{
			return decode<NV4097_SET_SCISSOR_VERTICAL>().height();
		}

		window_origin shader_window_origin() const
		{
			return decode<NV4097_SET_SHADER_WINDOW>().window_shader_origin();
		}

		window_pixel_center shader_window_pixel() const
		{
			return decode<NV4097_SET_SHADER_WINDOW>().window_shader_pixel_center();
		}

		u16 shader_window_height() const
		{
			return decode<NV4097_SET_SHADER_WINDOW>().window_shader_height();
		}

		u16 window_offset_x() const
		{
			return decode<NV4097_SET_WINDOW_OFFSET>().window_offset_x();
		}

		u16 window_offset_y() const
		{
			return decode<NV4097_SET_WINDOW_OFFSET>().window_offset_y();
		}

		u32 window_clip_type() const
		{
			return registers[NV4097_SET_WINDOW_CLIP_TYPE];
		}

		u32 window_clip_horizontal() const
		{
			return registers[NV4097_SET_WINDOW_CLIP_HORIZONTAL];
		}

		u32 window_clip_vertical() const
		{
			return registers[NV4097_SET_WINDOW_CLIP_HORIZONTAL];
		}

		bool depth_test_enabled() const
		{
			return decode<NV4097_SET_DEPTH_TEST_ENABLE>().depth_test_enabled();
		}

		bool depth_write_enabled() const
		{
			return decode<NV4097_SET_DEPTH_MASK>().depth_write_enabled();
		}

		bool alpha_test_enabled() const
		{
			return decode<NV4097_SET_ALPHA_TEST_ENABLE>().alpha_test_enabled();
		}

		bool stencil_test_enabled() const
		{
			return decode<NV4097_SET_STENCIL_TEST_ENABLE>().stencil_test_enabled();
		}

		u8 index_array_location() const
		{
			return decode<NV4097_SET_INDEX_ARRAY_DMA>().index_dma();
		}

		rsx::index_array_type index_type() const
		{
			return decode<NV4097_SET_INDEX_ARRAY_DMA>().type();
		}

		u32 restart_index() const
		{
			return decode<NV4097_SET_RESTART_INDEX>().restart_index();
		}

		bool restart_index_enabled_raw() const
		{
			return decode<NV4097_SET_RESTART_INDEX_ENABLE>().restart_index_enabled();
		}

		bool restart_index_enabled() const
		{
			if (!restart_index_enabled_raw())
			{
				return false;

			}

			if (index_type() == rsx::index_array_type::u16 &&
				restart_index() > 0xffff)
			{
				return false;
			}

			return true;
		}

		u32 z_clear_value(bool is_depth_stencil) const
		{
			return decode<NV4097_SET_ZSTENCIL_CLEAR_VALUE>().clear_z(is_depth_stencil);
		}

		u8 stencil_clear_value() const
		{
			return decode<NV4097_SET_ZSTENCIL_CLEAR_VALUE>().clear_stencil();
		}

		f32 fog_params_0() const
		{
			return decode<NV4097_SET_FOG_PARAMS>().fog_param_0();
		}

		f32 fog_params_1() const
		{
			return decode<NV4097_SET_FOG_PARAMS + 1>().fog_param_1();
		}

		bool color_mask_b(int index) const
		{
			if (index == 0)
			{
				return decode<NV4097_SET_COLOR_MASK>().color_b();
			}
			else
			{
				return decode<NV4097_SET_COLOR_MASK_MRT>().color_b(index);
			}
		}

		bool color_mask_g(int index) const
		{
			if (index == 0)
			{
				return decode<NV4097_SET_COLOR_MASK>().color_g();
			}
			else
			{
				return decode<NV4097_SET_COLOR_MASK_MRT>().color_g(index);
			}
		}

		bool color_mask_r(int index) const
		{
			if (index == 0)
			{
				return decode<NV4097_SET_COLOR_MASK>().color_r();
			}
			else
			{
				return decode<NV4097_SET_COLOR_MASK_MRT>().color_r(index);
			}
		}

		bool color_mask_a(int index) const
		{
			if (index == 0)
			{
				return decode<NV4097_SET_COLOR_MASK>().color_a();
			}
			else
			{
				return decode<NV4097_SET_COLOR_MASK_MRT>().color_a(index);
			}
		}

		bool color_write_enabled(int index) const
		{
			if (index == 0)
			{
				return decode<NV4097_SET_COLOR_MASK>().color_write_enabled();
			}
			else
			{
				return decode<NV4097_SET_COLOR_MASK_MRT>().color_write_enabled(index);
			}
		}

		u8 clear_color_b() const
		{
			return decode<NV4097_SET_COLOR_CLEAR_VALUE>().blue();
		}

		u8 clear_color_r() const
		{
			return decode<NV4097_SET_COLOR_CLEAR_VALUE>().red();
		}

		u8 clear_color_g() const
		{
			return decode<NV4097_SET_COLOR_CLEAR_VALUE>().green();
		}

		u8 clear_color_a() const
		{
			return decode<NV4097_SET_COLOR_CLEAR_VALUE>().alpha();
		}

		bool depth_bounds_test_enabled() const
		{
			return decode<NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE>().depth_bound_enabled();
		}

		f32 depth_bounds_min() const
		{
			return decode<NV4097_SET_DEPTH_BOUNDS_MIN>().depth_bound_min();
		}

		f32 depth_bounds_max() const
		{
			return decode<NV4097_SET_DEPTH_BOUNDS_MAX>().depth_bound_max();
		}

		f32 clip_min() const
		{
			return decode<NV4097_SET_CLIP_MIN>().clip_min();
		}

		f32 clip_max() const
		{
			return decode<NV4097_SET_CLIP_MAX>().clip_max();
		}

		bool logic_op_enabled() const
		{
			return decode<NV4097_SET_LOGIC_OP_ENABLE>().logic_op_enabled();
		}

		u8 stencil_mask() const
		{
			return decode<NV4097_SET_STENCIL_MASK>().stencil_mask();
		}

		u8 back_stencil_mask() const
		{
			return decode<NV4097_SET_BACK_STENCIL_MASK>().back_stencil_mask();
		}

		bool dither_enabled() const
		{
			return decode<NV4097_SET_DITHER_ENABLE>().dither_enabled();
		}

		bool blend_enabled() const
		{
			return decode<NV4097_SET_BLEND_ENABLE>().blend_enabled();
		}

		bool blend_enabled_surface_1() const
		{
			return decode<NV4097_SET_BLEND_ENABLE_MRT>().blend_surface_b();
		}

		bool blend_enabled_surface_2() const
		{
			return decode<NV4097_SET_BLEND_ENABLE_MRT>().blend_surface_c();
		}

		bool blend_enabled_surface_3() const
		{
			return decode<NV4097_SET_BLEND_ENABLE_MRT>().blend_surface_d();
		}

		bool line_smooth_enabled() const
		{
			return decode<NV4097_SET_LINE_SMOOTH_ENABLE>().line_smooth_enabled();
		}

		bool poly_offset_point_enabled() const
		{
			return decode<NV4097_SET_POLY_OFFSET_POINT_ENABLE>().poly_offset_point_enabled();
		}

		bool poly_offset_line_enabled() const
		{
			return decode<NV4097_SET_POLY_OFFSET_LINE_ENABLE>().poly_offset_line_enabled();
		}

		bool poly_offset_fill_enabled() const
		{
			return decode<NV4097_SET_POLY_OFFSET_FILL_ENABLE>().poly_offset_fill_enabled();
		}

		f32 poly_offset_scale() const
		{
			return decode<NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR>().polygon_offset_scale_factor();
		}

		f32 poly_offset_bias() const
		{
			return decode<NV4097_SET_POLYGON_OFFSET_BIAS>().polygon_offset_scale_bias();
		}

		bool cull_face_enabled() const
		{
			return decode<NV4097_SET_CULL_FACE_ENABLE>().cull_face_enabled();
		}

		bool poly_smooth_enabled() const
		{
			return decode<NV4097_SET_POLY_SMOOTH_ENABLE>().poly_smooth_enabled();
		}

		bool two_sided_stencil_test_enabled() const
		{
			return decode<NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE>().two_sided_stencil_test_enabled();
		}

		comparison_function depth_func() const
		{
			return decode<NV4097_SET_DEPTH_FUNC>().depth_func();
		}

		comparison_function stencil_func() const
		{
			return decode<NV4097_SET_STENCIL_FUNC>().stencil_func();
		}

		comparison_function back_stencil_func() const
		{
			return decode<NV4097_SET_BACK_STENCIL_FUNC>().back_stencil_func();
		}

		u8 stencil_func_ref() const
		{
			return decode<NV4097_SET_STENCIL_FUNC_REF>().stencil_func_ref();
		}

		u8 back_stencil_func_ref() const
		{
			return decode<NV4097_SET_BACK_STENCIL_FUNC_REF>().back_stencil_func_ref();
		}

		u8 stencil_func_mask() const
		{
			return decode<NV4097_SET_STENCIL_FUNC_MASK>().stencil_func_mask();
		}

		u8 back_stencil_func_mask() const
		{
			return decode<NV4097_SET_BACK_STENCIL_FUNC_MASK>().back_stencil_func_mask();
		}

		stencil_op stencil_op_fail() const
		{
			return decode<NV4097_SET_STENCIL_OP_FAIL>().fail();
		}

		stencil_op stencil_op_zfail() const
		{
			return decode<NV4097_SET_STENCIL_OP_ZFAIL>().zfail();
		}

		stencil_op stencil_op_zpass() const
		{
			return decode<NV4097_SET_STENCIL_OP_ZPASS>().zpass();
		}

		stencil_op back_stencil_op_fail() const
		{
			return decode<NV4097_SET_BACK_STENCIL_OP_FAIL>().back_fail();
		}

		rsx::stencil_op back_stencil_op_zfail() const
		{
			return decode<NV4097_SET_BACK_STENCIL_OP_ZFAIL>().back_zfail();
		}

		rsx::stencil_op back_stencil_op_zpass() const
		{
			return decode<NV4097_SET_BACK_STENCIL_OP_ZPASS>().back_zpass();
		}

		u8 blend_color_8b_r() const
		{
			return decode<NV4097_SET_BLEND_COLOR>().red8();
		}

		u8 blend_color_8b_g() const
		{
			return decode<NV4097_SET_BLEND_COLOR>().green8();
		}

		u8 blend_color_8b_b() const
		{
			return decode<NV4097_SET_BLEND_COLOR>().blue8();
		}

		u8 blend_color_8b_a() const
		{
			return decode<NV4097_SET_BLEND_COLOR>().alpha8();
		}

		u16 blend_color_16b_r() const
		{
			return decode<NV4097_SET_BLEND_COLOR>().red16();
		}

		u16 blend_color_16b_g() const
		{
			return decode<NV4097_SET_BLEND_COLOR>().green16();
		}

		u16 blend_color_16b_b() const
		{
			return decode<NV4097_SET_BLEND_COLOR2>().blue();
		}

		u16 blend_color_16b_a() const
		{
			return decode<NV4097_SET_BLEND_COLOR2>().alpha();
		}

		blend_equation blend_equation_rgb() const
		{
			return decode<NV4097_SET_BLEND_EQUATION>().blend_rgb();
		}

		blend_equation blend_equation_a() const
		{
			return decode<NV4097_SET_BLEND_EQUATION>().blend_a();
		}

		blend_factor blend_func_sfactor_rgb() const
		{
			return decode<NV4097_SET_BLEND_FUNC_SFACTOR>().src_blend_rgb();
		}

		blend_factor blend_func_sfactor_a() const
		{
			return decode<NV4097_SET_BLEND_FUNC_SFACTOR>().src_blend_a();
		}

		blend_factor blend_func_dfactor_rgb() const
		{
			return decode<NV4097_SET_BLEND_FUNC_DFACTOR>().dst_blend_rgb();
		}

		blend_factor blend_func_dfactor_a() const
		{
			return decode<NV4097_SET_BLEND_FUNC_DFACTOR>().dst_blend_a();
		}

		logic_op logic_operation() const
		{
			return decode<NV4097_SET_LOGIC_OP>().logic_operation();
		}

		user_clip_plane_op clip_plane_0_enabled() const
		{
			return decode<NV4097_SET_USER_CLIP_PLANE_CONTROL>().clip_plane0();
		}

		user_clip_plane_op clip_plane_1_enabled() const
		{
			return decode<NV4097_SET_USER_CLIP_PLANE_CONTROL>().clip_plane1();
		}

		user_clip_plane_op clip_plane_2_enabled() const
		{
			return decode<NV4097_SET_USER_CLIP_PLANE_CONTROL>().clip_plane2();
		}

		user_clip_plane_op clip_plane_3_enabled() const
		{
			return decode<NV4097_SET_USER_CLIP_PLANE_CONTROL>().clip_plane3();
		}

		user_clip_plane_op clip_plane_4_enabled() const
		{
			return decode<NV4097_SET_USER_CLIP_PLANE_CONTROL>().clip_plane4();
		}

		user_clip_plane_op clip_plane_5_enabled() const
		{
			return decode<NV4097_SET_USER_CLIP_PLANE_CONTROL>().clip_plane5();
		}

		front_face front_face_mode() const
		{
			return decode<NV4097_SET_FRONT_FACE>().front_face_mode();
		}

		cull_face cull_face_mode() const
		{
			return decode<NV4097_SET_CULL_FACE>().cull_face_mode();
		}

		f32 line_width() const
		{
			return decode<NV4097_SET_LINE_WIDTH>().line_width();
		}

		f32 point_size() const
		{
			return decode<NV4097_SET_POINT_SIZE>().point_size();
		}

		bool point_sprite_enabled() const
		{
			return decode<NV4097_SET_POINT_SPRITE_CONTROL>().enabled();
		}

		u8 alpha_ref() const
		{
			return decode<NV4097_SET_ALPHA_REF>().alpha_ref();
		}

		surface_target surface_color_target() const
		{
			return decode<NV4097_SET_SURFACE_COLOR_TARGET>().target();
		}

		u16 surface_clip_origin_x() const
		{
			return decode<NV4097_SET_SURFACE_CLIP_HORIZONTAL>().origin_x();
		}

		u16 surface_clip_width() const
		{
			return decode<NV4097_SET_SURFACE_CLIP_HORIZONTAL>().width();
		}

		u16 surface_clip_origin_y() const
		{
			return decode<NV4097_SET_SURFACE_CLIP_VERTICAL>().origin_y();
		}

		u16 surface_clip_height() const
		{
			return decode<NV4097_SET_SURFACE_CLIP_VERTICAL>().height();
		}

		u32 surface_a_offset() const
		{
			return decode<NV4097_SET_SURFACE_COLOR_AOFFSET>().surface_a_offset();
		}

		u32 surface_b_offset() const
		{
			return decode<NV4097_SET_SURFACE_COLOR_BOFFSET>().surface_b_offset();
		}

		u32 surface_c_offset() const
		{
			return decode<NV4097_SET_SURFACE_COLOR_COFFSET>().surface_c_offset();
		}

		u32 surface_d_offset() const
		{
			return decode<NV4097_SET_SURFACE_COLOR_DOFFSET>().surface_d_offset();
		}

		u32 surface_a_pitch() const
		{
			return decode<NV4097_SET_SURFACE_PITCH_A>().surface_a_pitch();
		}

		u32 surface_b_pitch() const
		{
			return decode<NV4097_SET_SURFACE_PITCH_B>().surface_b_pitch();
		}

		u32 surface_c_pitch() const
		{
			return decode<NV4097_SET_SURFACE_PITCH_C>().surface_c_pitch();
		}

		u32 surface_d_pitch() const
		{
			return decode<NV4097_SET_SURFACE_PITCH_D>().surface_d_pitch();
		}

		u32 surface_a_dma() const
		{
			return decode<NV4097_SET_CONTEXT_DMA_COLOR_A>().dma_surface_a();
		}

		u32 surface_b_dma() const
		{
			return decode<NV4097_SET_CONTEXT_DMA_COLOR_B>().dma_surface_b();
		}

		u32 surface_c_dma() const
		{
			return decode<NV4097_SET_CONTEXT_DMA_COLOR_C>().dma_surface_c();
		}

		u32 surface_d_dma() const
		{
			return decode<NV4097_SET_CONTEXT_DMA_COLOR_D>().dma_surface_d();
		}

		u32 surface_z_offset() const
		{
			return decode<NV4097_SET_SURFACE_ZETA_OFFSET>().surface_z_offset();
		}

		u32 surface_z_pitch() const
		{
			return decode<NV4097_SET_SURFACE_PITCH_Z>().surface_z_pitch();
		}

		u32 surface_z_dma() const
		{
			return decode<NV4097_SET_CONTEXT_DMA_ZETA>().dma_surface_z();
		}

		f32 viewport_scale_x() const
		{
			return decode<NV4097_SET_VIEWPORT_SCALE>().viewport_scale_x();
		}

		f32 viewport_scale_y() const
		{
			return decode<NV4097_SET_VIEWPORT_SCALE + 1>().viewport_scale_y();
		}

		f32 viewport_scale_z() const
		{
			return decode<NV4097_SET_VIEWPORT_SCALE + 2>().viewport_scale_z();
		}

		f32 viewport_scale_w() const
		{
			return decode<NV4097_SET_VIEWPORT_SCALE + 3>().viewport_scale_w();
		}

		f32 viewport_offset_x() const
		{
			return decode<NV4097_SET_VIEWPORT_OFFSET>().viewport_offset_x();
		}

		f32 viewport_offset_y() const
		{
			return decode<NV4097_SET_VIEWPORT_OFFSET + 1>().viewport_offset_y();
		}

		f32 viewport_offset_z() const
		{
			return decode<NV4097_SET_VIEWPORT_OFFSET + 2>().viewport_offset_z();
		}

		f32 viewport_offset_w() const
		{
			return decode<NV4097_SET_VIEWPORT_OFFSET + 3>().viewport_offset_w();
		}

		bool two_side_light_en() const
		{
			return decode<NV4097_SET_TWO_SIDE_LIGHT_EN>().two_sided_lighting_enabled();
		}

		fog_mode fog_equation() const
		{
			return decode<NV4097_SET_FOG_MODE>().fog_equation();
		}

		comparison_function alpha_func() const
		{
			return decode<NV4097_SET_ALPHA_FUNC>().alpha_func();
		}

		u16 vertex_attrib_input_mask() const
		{
			return decode<NV4097_SET_VERTEX_ATTRIB_INPUT_MASK>().mask();
		}

		u16 frequency_divider_operation_mask() const
		{
			return decode<NV4097_SET_FREQUENCY_DIVIDER_OPERATION>().frequency_divider_operation_mask();
		}

		u32 vertex_attrib_output_mask() const
		{
			return decode<NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK>().output_mask();
		}

		u32 shader_control() const
		{
			return decode<NV4097_SET_SHADER_CONTROL>().shader_ctrl();
		}

		surface_color_format surface_color() const
		{
			return decode<NV4097_SET_SURFACE_FORMAT>().color_fmt();
		}

		surface_depth_format surface_depth_fmt() const
		{
			return decode<NV4097_SET_SURFACE_FORMAT>().depth_fmt();
		}

		surface_raster_type surface_type() const
		{
			return decode<NV4097_SET_SURFACE_FORMAT>().type();
		}

		surface_antialiasing surface_antialias() const
		{
			return decode<NV4097_SET_SURFACE_FORMAT>().antialias();
		}

		u8 surface_log2_height() const
		{
			return decode<NV4097_SET_SURFACE_FORMAT>().log2height();
		}

		u8 surface_log2_width() const
		{
			return decode<NV4097_SET_SURFACE_FORMAT>().log2width();
		}

		u32 vertex_data_base_offset() const
		{
			return decode<NV4097_SET_VERTEX_DATA_BASE_OFFSET>().vertex_data_base_offset();
		}

		u32 index_array_address() const
		{
			return decode<NV4097_SET_INDEX_ARRAY_ADDRESS>().index_array_offset();
		}

		u32 vertex_data_base_index() const
		{
			return decode<NV4097_SET_VERTEX_DATA_BASE_INDEX>().vertex_data_base_index();
		}

		u32 shader_program_address() const
		{
			return decode<NV4097_SET_SHADER_PROGRAM>().shader_program_address();
		}

		u32 transform_program_start() const
		{
			return decode<NV4097_SET_TRANSFORM_PROGRAM_START>().transform_program_start();
		}

		primitive_type primitive_mode() const
		{
			return decode<NV4097_SET_BEGIN_END>().primitive();
		}

		u32 semaphore_context_dma_406e() const
		{
			return decode<NV406E_SET_CONTEXT_DMA_SEMAPHORE>().context_dma();
		}

		u32 semaphore_offset_406e() const
		{
			return decode<NV406E_SEMAPHORE_OFFSET>().semaphore_offset();
		}

		u32 semaphore_context_dma_4097() const
		{
			return decode<NV4097_SET_CONTEXT_DMA_SEMAPHORE>().context_dma();
		}

		u32 semaphore_offset_4097() const
		{
			return decode<NV4097_SET_SEMAPHORE_OFFSET>().semaphore_offset();
		}

		blit_engine::context_dma context_dma_report() const
		{
			return decode<NV4097_SET_CONTEXT_DMA_REPORT>().context_dma_report();
		}

		u32 context_dma_notify() const
		{
			return decode<NV4097_SET_CONTEXT_DMA_NOTIFIES>().context_dma_notify();
		}

		blit_engine::transfer_operation blit_engine_operation() const
		{
			return decode<NV3089_SET_OPERATION>().transfer_op();
		}

		/// TODO: find the purpose vs in/out equivalents
		u16 blit_engine_clip_x() const
		{
			return decode<NV3089_CLIP_POINT>().clip_x();
		}

		u16 blit_engine_clip_y() const
		{
			return decode<NV3089_CLIP_POINT>().clip_y();
		}

		u16 blit_engine_clip_width() const
		{
			return decode<NV3089_CLIP_SIZE>().clip_width();
		}

		u16 blit_engine_clip_height() const
		{
			return decode<NV3089_CLIP_SIZE>().clip_height();
		}

		u16 blit_engine_output_x() const
		{
			return decode<NV3089_IMAGE_OUT_POINT>().x();
		}

		u16 blit_engine_output_y() const
		{
			return decode<NV3089_IMAGE_OUT_POINT>().y();
		}

		u16 blit_engine_output_width() const
		{
			return decode<NV3089_IMAGE_OUT_SIZE>().width();
		}

		u16 blit_engine_output_height() const
		{
			return decode<NV3089_IMAGE_OUT_SIZE>().height();
		}

		// there is no x/y ?
		u16 blit_engine_input_width() const
		{
			return decode<NV3089_IMAGE_IN_SIZE>().width();
		}

		u16 blit_engine_input_height() const
		{
			return decode<NV3089_IMAGE_IN_SIZE>().height();
		}

		u16 blit_engine_input_pitch() const
		{
			return decode<NV3089_IMAGE_IN_FORMAT>().format();
		}

		blit_engine::transfer_origin blit_engine_input_origin() const
		{
			return decode<NV3089_IMAGE_IN_FORMAT>().transfer_origin();
		}

		blit_engine::transfer_interpolator blit_engine_input_inter() const
		{
			return decode<NV3089_IMAGE_IN_FORMAT>().transfer_interpolator();
		}

		blit_engine::transfer_source_format blit_engine_src_color_format() const
		{
			return decode<NV3089_SET_COLOR_FORMAT>().transfer_source_fmt();
		}

		// ???
		f32 blit_engine_in_x() const
		{
			return decode<NV3089_IMAGE_IN>().x();
		}

		// ???
		f32 blit_engine_in_y() const
		{
			return decode<NV3089_IMAGE_IN>().y();
		}

		u32 blit_engine_input_offset() const
		{
			return decode<NV3089_IMAGE_IN_OFFSET>().input_offset();
		}

		u32 blit_engine_input_location() const
		{
			return decode<NV3089_SET_CONTEXT_DMA_IMAGE>().context_dma();
		}

		blit_engine::context_surface blit_engine_context_surface() const
		{
			return decode<NV3089_SET_CONTEXT_SURFACE>().ctx_surface();
		}

		u32 blit_engine_output_location_nv3062() const
		{
			return decode<NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN>().output_dma();
		}

		u32 blit_engine_output_offset_nv3062() const
		{
			return decode<NV3062_SET_OFFSET_DESTIN>().output_offset();
		}

		blit_engine::transfer_destination_format blit_engine_nv3062_color_format() const
		{
			return decode<NV3062_SET_COLOR_FORMAT>().transfer_dest_fmt();
		}

		u16 blit_engine_output_alignment_nv3062() const
		{
			return decode<NV3062_SET_PITCH>().alignment();
		}

		u16 blit_engine_output_pitch_nv3062() const
		{
			return decode<NV3062_SET_PITCH>().pitch();
		}

		u32 blit_engine_nv309E_location() const
		{
			return decode<NV309E_SET_CONTEXT_DMA_IMAGE>().context_dma();
		}

		u32 blit_engine_nv309E_offset() const
		{
			return decode<NV309E_SET_OFFSET>().offset();
		}

		blit_engine::transfer_destination_format blit_engine_output_format_nv309E() const
		{
			return decode<NV309E_SET_FORMAT>().format();
		}

		f32 blit_engine_ds_dx() const
		{
			return decode<NV3089_DS_DX>().ds_dx();
		}

		f32 blit_engine_dt_dy() const
		{
			return decode<NV3089_DT_DY>().dt_dy();
		}

		u8 nv309e_sw_width_log2() const
		{
			return decode<NV309E_SET_FORMAT>().sw_width_log2();
		}

		u8 nv309e_sw_height_log2() const
		{
			return decode<NV309E_SET_FORMAT>().sw_height_log2();
		}

		u32 nv0039_input_pitch() const
		{
			return decode<NV0039_PITCH_IN>().input_pitch();
		}

		u32 nv0039_output_pitch() const
		{
			return decode<NV0039_PITCH_OUT>().output_pitch();
		}

		u32 nv0039_line_length() const
		{
			return decode<NV0039_LINE_LENGTH_IN>().input_line_length();
		}

		u32 nv0039_line_count() const
		{
			return decode<NV0039_LINE_COUNT>().line_count();
		}

		u8 nv0039_output_format() const
		{
			return decode<NV0039_FORMAT>().output_format();
		}

		u8 nv0039_input_format() const
		{
			return decode<NV0039_FORMAT>().input_format();
		}

		u32 nv0039_output_offset() const
		{
			return decode<NV0039_OFFSET_OUT>().output_offset();
		}

		u32 nv0039_output_location()
		{
			return decode<NV0039_SET_CONTEXT_DMA_BUFFER_OUT>().output_dma();
		}

		u32 nv0039_input_offset() const
		{
			return decode<NV0039_OFFSET_IN>().input_offset();
		}

		u32 nv0039_input_location() const
		{
			return decode<NV0039_SET_CONTEXT_DMA_BUFFER_IN>().input_dma();
		}

		u16 nv308a_x() const
		{
			return decode<NV308A_POINT>().x();
		}

		u16 nv308a_y() const
		{
			return decode<NV308A_POINT>().y();
		}

		u16 nv308a_size_in_x() const
		{
			return u16(registers[NV308A_SIZE_IN] & 0xFFFF);
		}

		u16 nv308a_size_out_x() const
		{
			return u16(registers[NV308A_SIZE_OUT] & 0xFFFF);
		}

		u32 transform_program_load()
		{
			return registers[NV4097_SET_TRANSFORM_PROGRAM_LOAD];
		}

		void commit_4_transform_program_instructions(u32 index)
		{
			u32& load = registers[NV4097_SET_TRANSFORM_PROGRAM_LOAD];

			transform_program[load * 4] = registers[NV4097_SET_TRANSFORM_PROGRAM + index * 4];
			transform_program[load * 4 + 1] = registers[NV4097_SET_TRANSFORM_PROGRAM + index * 4 + 1];
			transform_program[load * 4 + 2] = registers[NV4097_SET_TRANSFORM_PROGRAM + index * 4 + 2];
			transform_program[load * 4 + 3] = registers[NV4097_SET_TRANSFORM_PROGRAM + index * 4 + 3];
			load++;
		}

		u32 transform_constant_load()
		{
			return registers[NV4097_SET_TRANSFORM_CONSTANT_LOAD];
		}

		u32 transform_branch_bits()
		{
			return registers[NV4097_SET_TRANSFORM_BRANCH_BITS];
		}

		u16 msaa_sample_mask()
		{
			return decode<NV4097_SET_ANTI_ALIASING_CONTROL>().msaa_sample_mask();
		}

		bool msaa_enabled()
		{
			return decode<NV4097_SET_ANTI_ALIASING_CONTROL>().msaa_enabled();
		}

		bool msaa_alpha_to_coverage_enabled()
		{
			return decode<NV4097_SET_ANTI_ALIASING_CONTROL>().msaa_alpha_to_coverage();
		}

		bool msaa_alpha_to_one_enabled()
		{
			return decode<NV4097_SET_ANTI_ALIASING_CONTROL>().msaa_alpha_to_one();
		}

		bool depth_clamp_enabled()
		{
			return decode<NV4097_SET_ZMIN_MAX_CONTROL>().depth_clamp_enabled();
		}

		bool depth_clip_enabled()
		{
			return decode<NV4097_SET_ZMIN_MAX_CONTROL>().depth_clip_enabled();
		}

		bool depth_clip_ignore_w()
		{
			return decode<NV4097_SET_ZMIN_MAX_CONTROL>().depth_clip_ignore_w();
		}

		bool framebuffer_srgb_enabled()
		{
			return decode<NV4097_SET_SHADER_PACKER>().srgb_output_enabled();
		}

		bool depth_buffer_float_enabled()
		{
			return decode<NV4097_SET_CONTROL0>().depth_float();
		}

		u16 texcoord_control_mask() const
		{
			// Only 10 texture coords exist [0-9]
			u16 control_mask = 0;
			for (u8 index = 0; index < 10; ++index)
			{
				control_mask |= ((registers[NV4097_SET_TEX_COORD_CONTROL + index] & 1) << index);
			}

			return control_mask;
		}

		u16 point_sprite_control_mask() const
		{
			return decode<NV4097_SET_POINT_SPRITE_CONTROL>().texcoord_mask();
		}
	};

	extern rsx_state method_registers;
	extern std::array<rsx_method_t, 0x10000 / 4> methods;
}
