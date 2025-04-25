#include "stdafx.h"
#include "RSXDrawCommands.h"

#include "Emu/RSX/Common/BufferUtils.h"
#include "Emu/RSX/Common/buffer_stream.hpp"
#include "Emu/RSX/Common/io_buffer.h"
#include "Emu/RSX/NV47/HW/context_accessors.define.h"
#include "Emu/RSX/Program/GLSLCommon.h"
#include "Emu/RSX/rsx_methods.h"
#include "Emu/RSX/RSXThread.h"

#include "Emu/Memory/vm.h"

namespace rsx
{
	void draw_command_processor::analyse_inputs_interleaved(vertex_input_layout& result, const vertex_program_metadata_t& vp_metadata)
	{
		const rsx_state& state = *REGS(m_ctx);
		const u32 input_mask = state.vertex_attrib_input_mask() & vp_metadata.referenced_inputs_mask;

		result.clear();
		result.attribute_mask = static_cast<u16>(input_mask);

		if (state.current_draw_clause.command == rsx::draw_command::inlined_array)
		{
			interleaved_range_info& info = *result.alloc_interleaved_block();
			info.interleaved = true;

			for (u8 index = 0; index < rsx::limits::vertex_count; ++index)
			{
				auto& vinfo = state.vertex_arrays_info[index];
				result.attribute_placement[index] = attribute_buffer_placement::none;

				if (vinfo.size() > 0)
				{
					// Stride must be updated even if the stream is disabled
					info.attribute_stride += rsx::get_vertex_type_size_on_host(vinfo.type(), vinfo.size());
					info.locations.push_back({ index, false, 1 });

					if (input_mask & (1u << index))
					{
						result.attribute_placement[index] = attribute_buffer_placement::transient;
					}
				}
				else if (state.register_vertex_info[index].size > 0 && input_mask & (1u << index))
				{
					// Reads from register
					result.referenced_registers.push_back(index);
					result.attribute_placement[index] = attribute_buffer_placement::transient;
				}
			}

			if (info.attribute_stride)
			{
				// At least one array feed must be enabled for vertex input
				result.interleaved_blocks.push_back(&info);
			}

			return;
		}

		const u32 frequency_divider_mask = REGS(m_ctx)->frequency_divider_operation_mask();
		result.interleaved_blocks.reserve(16);
		result.referenced_registers.reserve(16);

		for (auto [ref_mask, index] = std::tuple{ input_mask, u8(0) }; ref_mask; ++index, ref_mask >>= 1)
		{
			ensure(index < rsx::limits::vertex_count);

			if (!(ref_mask & 1u))
			{
				// Nothing to do, uninitialized
				continue;
			}

			// Always reset attribute placement by default
			result.attribute_placement[index] = attribute_buffer_placement::none;

			// Check for interleaving
			if (REGS(m_ctx)->current_draw_clause.is_immediate_draw &&
				REGS(m_ctx)->current_draw_clause.command != rsx::draw_command::indexed)
			{
				// NOTE: In immediate rendering mode, all vertex setup is ignored
				// Observed with GT5, immediate render bypasses array pointers completely, even falling back to fixed-function register defaults
				if (m_vertex_push_buffers[index].vertex_count > 1)
				{
					// Ensure consistent number of vertices per attribute.
					m_vertex_push_buffers[index].pad_to(m_vertex_push_buffers[0].vertex_count, false);

					// Read temp buffer (register array)
					std::pair<u8, u32> volatile_range_info = std::make_pair(index, static_cast<u32>(m_vertex_push_buffers[index].data.size() * sizeof(u32)));
					result.volatile_blocks.push_back(volatile_range_info);
					result.attribute_placement[index] = attribute_buffer_placement::transient;
				}
				else if (state.register_vertex_info[index].size > 0)
				{
					// Reads from register
					result.referenced_registers.push_back(index);
					result.attribute_placement[index] = attribute_buffer_placement::transient;
				}

				// Fall back to the default register value if no source is specified via register
				continue;
			}

			const auto& info = state.vertex_arrays_info[index];
			if (!info.size())
			{
				if (state.register_vertex_info[index].size > 0)
				{
					// Reads from register
					result.referenced_registers.push_back(index);
					result.attribute_placement[index] = attribute_buffer_placement::transient;
					continue;
				}
			}
			else
			{
				result.attribute_placement[index] = attribute_buffer_placement::persistent;
				const u32 base_address = info.offset() & 0x7fffffff;
				bool alloc_new_block = true;
				bool modulo = !!(frequency_divider_mask & (1 << index));

				for (auto& block : result.interleaved_blocks)
				{
					if (block->single_vertex)
					{
						// Single vertex definition, continue
						continue;
					}

					if (block->attribute_stride != info.stride())
					{
						// Stride does not match, continue
						continue;
					}

					if (base_address > block->base_offset)
					{
						const u32 diff = base_address - block->base_offset;
						if (diff > info.stride())
						{
							// Not interleaved, continue
							continue;
						}
					}
					else
					{
						const u32 diff = block->base_offset - base_address;
						if (diff > info.stride())
						{
							// Not interleaved, continue
							continue;
						}

						// Matches, and this address is lower than existing
						block->base_offset = base_address;
					}

					alloc_new_block = false;
					block->locations.push_back({ index, modulo, info.frequency() });
					block->interleaved = true;
					break;
				}

				if (alloc_new_block)
				{
					interleaved_range_info& block = *result.alloc_interleaved_block();
					block.base_offset = base_address;
					block.attribute_stride = info.stride();
					block.memory_location = info.offset() >> 31;
					block.locations.reserve(16);
					block.locations.push_back({ index, modulo, info.frequency() });

					if (block.attribute_stride == 0)
					{
						block.single_vertex = true;
						block.attribute_stride = rsx::get_vertex_type_size_on_host(info.type(), info.size());
					}

					result.interleaved_blocks.push_back(&block);
				}
			}
		}

		for (auto& info : result.interleaved_blocks)
		{
			// Calculate real data address to be used during upload
			info->real_offset_address = rsx::get_address(rsx::get_vertex_offset_from_base(state.vertex_data_base_offset(), info->base_offset), info->memory_location);
		}
	}

	std::span<const std::byte> draw_command_processor::get_raw_index_array(const draw_clause& draw_indexed_clause) const
	{
		if (!m_element_push_buffer.empty()) [[ unlikely ]]
		{
			// Indices provided via immediate mode
			return { reinterpret_cast<const std::byte*>(m_element_push_buffer.data()), ::narrow<u32>(m_element_push_buffer.size() * sizeof(u32)) };
		}

		const rsx::index_array_type type = REGS(m_ctx)->index_type();
		const u32 type_size = get_index_type_size(type);

		// Force aligned indices as realhw
		const u32 address = (0 - type_size) & get_address(REGS(m_ctx)->index_array_address(), REGS(m_ctx)->index_array_location());

		const u32 first = draw_indexed_clause.min_index();
		const u32 count = draw_indexed_clause.get_elements_count();

		const auto ptr = vm::_ptr<const std::byte>(address);
		return { ptr + first * type_size, count * type_size };
	}

	std::variant<draw_array_command, draw_indexed_array_command, draw_inlined_array>
		draw_command_processor::get_draw_command(const rsx::rsx_state& state) const
	{
		if (REGS(m_ctx)->current_draw_clause.command == rsx::draw_command::indexed) [[ likely ]]
		{
			return draw_indexed_array_command
			{
				get_raw_index_array(state.current_draw_clause)
			};
		}

		if (REGS(m_ctx)->current_draw_clause.command == rsx::draw_command::array)
		{
			return draw_array_command{};
		}

		if (REGS(m_ctx)->current_draw_clause.command == rsx::draw_command::inlined_array)
		{
			return draw_inlined_array{};
		}

		fmt::throw_exception("ill-formed draw command");
	}

	void draw_command_processor::append_to_push_buffer(u32 attribute, u32 size, u32 subreg_index, vertex_base_type type, u32 value)
	{
		if (!(REGS(m_ctx)->vertex_attrib_input_mask() & (1 << attribute)))
		{
			return;
		}

		// Enforce ATTR0 as vertex attribute for push buffers.
		// This whole thing becomes a mess if we don't have a provoking attribute.
		const auto vertex_id = m_vertex_push_buffers[0].get_vertex_id();
		m_vertex_push_buffers[attribute].set_vertex_data(attribute, vertex_id, subreg_index, type, size, value);
		RSX(m_ctx)->m_graphics_state |= rsx::pipeline_state::push_buffer_arrays_dirty;
	}

	u32 draw_command_processor::get_push_buffer_vertex_count() const
	{
		// Enforce ATTR0 as vertex attribute for push buffers.
		// This whole thing becomes a mess if we don't have a provoking attribute.
		return m_vertex_push_buffers[0].vertex_count;
	}

	void draw_command_processor::append_array_element(u32 index)
	{
		// Endianness is swapped because common upload code expects input in BE
		// TODO: Implement fast upload path for LE inputs and do away with this
		m_element_push_buffer.push_back(std::bit_cast<u32, be_t<u32>>(index));
	}

	u32 draw_command_processor::get_push_buffer_index_count() const
	{
		return ::size32(m_element_push_buffer);
	}

	void draw_command_processor::clear_push_buffers()
	{
		auto& graphics_state = RSX(m_ctx)->m_graphics_state;
		if (graphics_state & rsx::pipeline_state::push_buffer_arrays_dirty)
		{
			for (auto& push_buf : m_vertex_push_buffers)
			{
				//Disabled, see https://github.com/RPCS3/rpcs3/issues/1932
				//REGS(m_ctx)->register_vertex_info[index].size = 0;

				push_buf.clear();
			}

			graphics_state.clear(rsx::pipeline_state::push_buffer_arrays_dirty);
		}

		m_element_push_buffer.clear();
	}

	void draw_command_processor::fill_vertex_layout_state(
		const vertex_input_layout& layout,
		const vertex_program_metadata_t& vp_metadata,
		u32 first_vertex,
		u32 vertex_count,
		s32* buffer,
		u32 persistent_offset_base,
		u32 volatile_offset_base) const
	{
		std::array<s32, 16> offset_in_block = {};
		u32 volatile_offset = volatile_offset_base;
		u32 persistent_offset = persistent_offset_base;

		// NOTE: Order is important! Transient ayout is always push_buffers followed by register data
		if (REGS(m_ctx)->current_draw_clause.is_immediate_draw)
		{
			for (const auto& info : layout.volatile_blocks)
			{
				offset_in_block[info.first] = volatile_offset;
				volatile_offset += info.second;
			}
		}

		for (u8 index : layout.referenced_registers)
		{
			offset_in_block[index] = volatile_offset;
			volatile_offset += 16;
		}

		if (REGS(m_ctx)->current_draw_clause.command == rsx::draw_command::inlined_array)
		{
			const auto& block = layout.interleaved_blocks[0];
			u32 inline_data_offset = volatile_offset;
			for (const auto& attrib : block->locations)
			{
				auto& info = REGS(m_ctx)->vertex_arrays_info[attrib.index];

				offset_in_block[attrib.index] = inline_data_offset;
				inline_data_offset += rsx::get_vertex_type_size_on_host(info.type(), info.size());
			}
		}
		else
		{
			for (const auto& block : layout.interleaved_blocks)
			{
				for (const auto& attrib : block->locations)
				{
					const u32 local_address = (REGS(m_ctx)->vertex_arrays_info[attrib.index].offset() & 0x7fffffff);
					offset_in_block[attrib.index] = persistent_offset + (local_address - block->base_offset);
				}

				const auto range = block->calculate_required_range(first_vertex, vertex_count);
				persistent_offset += block->attribute_stride * range.second;
			}
		}

		// Fill the data
		// Each descriptor field is 64 bits wide
		// [0-8] attribute stride
		// [8-24] attribute divisor
		// [24-27] attribute type
		// [27-30] attribute size
		// [30-31] reserved
		// [31-60] starting offset
		// [60-21] swap bytes flag
		// [61-22] volatile flag
		// [62-63] modulo enable flag

		const s32 default_frequency_mask = (1 << 8);
		const s32 swap_storage_mask = (1 << 29);
		const s32 volatile_storage_mask = (1 << 30);
		const s32 modulo_op_frequency_mask = smin;

		const u32 modulo_mask = REGS(m_ctx)->frequency_divider_operation_mask();
		const auto max_index = (first_vertex + vertex_count) - 1;

		for (u16 ref_mask = vp_metadata.referenced_inputs_mask, index = 0; ref_mask; ++index, ref_mask >>= 1)
		{
			if (!(ref_mask & 1u))
			{
				// Unused input, ignore this
				continue;
			}

			if (layout.attribute_placement[index] == attribute_buffer_placement::none)
			{
				static constexpr u64 zero = 0;
				std::memcpy(buffer + index * 2, &zero, sizeof(zero));
				continue;
			}

			rsx::vertex_base_type type = {};
			s32 size = 0;
			s32 attrib0 = 0;
			s32 attrib1 = 0;

			if (layout.attribute_placement[index] == attribute_buffer_placement::transient)
			{
				if (REGS(m_ctx)->current_draw_clause.command == rsx::draw_command::inlined_array)
				{
					const auto& info = REGS(m_ctx)->vertex_arrays_info[index];

					if (!info.size())
					{
						// Register
						const auto& reginfo = REGS(m_ctx)->register_vertex_info[index];
						type = reginfo.type;
						size = reginfo.size;

						attrib0 = rsx::get_vertex_type_size_on_host(type, size);
					}
					else
					{
						// Array
						type = info.type();
						size = info.size();

						attrib0 = layout.interleaved_blocks[0]->attribute_stride | default_frequency_mask;
					}
				}
				else
				{
					// Data is either from an immediate render or register input
					// Immediate data overrides register input

					if (REGS(m_ctx)->current_draw_clause.is_immediate_draw &&
						m_vertex_push_buffers[index].vertex_count > 1)
					{
						// Push buffer
						const auto& info = m_vertex_push_buffers[index];
						type = info.type;
						size = info.size;

						attrib0 = rsx::get_vertex_type_size_on_host(type, size) | default_frequency_mask;
					}
					else
					{
						// Register
						const auto& info = REGS(m_ctx)->register_vertex_info[index];
						type = info.type;
						size = info.size;

						attrib0 = rsx::get_vertex_type_size_on_host(type, size);
					}
				}

				attrib1 |= volatile_storage_mask;
			}
			else
			{
				auto& info = REGS(m_ctx)->vertex_arrays_info[index];
				type = info.type();
				size = info.size();

				auto stride = info.stride();
				attrib0 = stride;

				if (stride > 0) // when stride is 0, input is not an array but a single element
				{
					const u32 frequency = info.frequency();
					switch (frequency)
					{
					case 0:
					case 1:
					{
						attrib0 |= default_frequency_mask;
						break;
					}
					default:
					{
						if (modulo_mask & (1 << index))
						{
							if (max_index >= frequency)
							{
								// Only set modulo mask if a modulo op is actually necessary!
								// This requires that the uploaded range for this attr = [0, freq-1]
								// Ignoring modulo op if the rendered range does not wrap allows for range optimization
								attrib0 |= (frequency << 8);
								attrib1 |= modulo_op_frequency_mask;
							}
							else
							{
								attrib0 |= default_frequency_mask;
							}
						}
						else
						{
							// Division
							attrib0 |= (frequency << 8);
						}
						break;
					}
					}
				}
			} // end attribute placement check

			// Special compressed 4 components into one 4-byte value. Decoded as one value.
			if (type == rsx::vertex_base_type::cmp)
			{
				size = 1;
			}

			// All data is passed in in PS3-native order (BE) so swap flag should be set
			attrib1 |= swap_storage_mask;
			attrib0 |= (static_cast<s32>(type) << 24);
			attrib0 |= (size << 27);
			attrib1 |= offset_in_block[index];

			buffer[index * 2 + 0] = attrib0;
			buffer[index * 2 + 1] = attrib1;
		}
	}

	void draw_command_processor::write_vertex_data_to_memory(
		const vertex_input_layout& layout,
		u32 first_vertex,
		u32 vertex_count,
		void* persistent_data,
		void* volatile_data) const
	{
		auto transient = static_cast<char*>(volatile_data);
		auto persistent = static_cast<char*>(persistent_data);

		auto& draw_call = REGS(m_ctx)->current_draw_clause;

		if (transient != nullptr)
		{
			if (draw_call.command == rsx::draw_command::inlined_array)
			{
				for (const u8 index : layout.referenced_registers)
				{
					memcpy(transient, REGS(m_ctx)->register_vertex_info[index].data.data(), 16);
					transient += 16;
				}

				memcpy(transient, draw_call.inline_vertex_array.data(), draw_call.inline_vertex_array.size() * sizeof(u32));
				// Is it possible to reference data outside of the inlined array?
				return;
			}

			// NOTE: Order is important! Transient layout is always push_buffers followed by register data
			if (draw_call.is_immediate_draw)
			{
				// NOTE: It is possible for immediate draw to only contain index data, so vertex data can be in persistent memory
				for (const auto& info : layout.volatile_blocks)
				{
					memcpy(transient, m_vertex_push_buffers[info.first].data.data(), info.second);
					transient += info.second;
				}
			}

			for (const u8 index : layout.referenced_registers)
			{
				memcpy(transient, REGS(m_ctx)->register_vertex_info[index].data.data(), 16);
				transient += 16;
			}
		}

		if (persistent != nullptr)
		{
			for (interleaved_range_info* block : layout.interleaved_blocks)
			{
				auto range = block->calculate_required_range(first_vertex, vertex_count);

				const u32 data_size = range.second * block->attribute_stride;
				const u32 vertex_base = range.first * block->attribute_stride;

				g_fxo->get<rsx::dma_manager>().copy(persistent, vm::_ptr<char>(block->real_offset_address) + vertex_base, data_size);
				persistent += data_size;
			}
		}
	}

	void draw_command_processor::fill_scale_offset_data(void* buffer, bool flip_y) const
	{
		const int clip_w = REGS(m_ctx)->surface_clip_width();
		const int clip_h = REGS(m_ctx)->surface_clip_height();

		const float scale_x = REGS(m_ctx)->viewport_scale_x() / (clip_w / 2.f);
		float offset_x = REGS(m_ctx)->viewport_offset_x() - (clip_w / 2.f);
		offset_x /= clip_w / 2.f;

		float scale_y = REGS(m_ctx)->viewport_scale_y() / (clip_h / 2.f);
		float offset_y = (REGS(m_ctx)->viewport_offset_y() - (clip_h / 2.f));
		offset_y /= clip_h / 2.f;
		if (flip_y) scale_y *= -1;
		if (flip_y) offset_y *= -1;

		const float scale_z = REGS(m_ctx)->viewport_scale_z();
		const float offset_z = REGS(m_ctx)->viewport_offset_z();
		const float one = 1.f;

		utils::stream_vector(buffer, std::bit_cast<u32>(scale_x), 0, 0, std::bit_cast<u32>(offset_x));
		utils::stream_vector(static_cast<char*>(buffer) + 16, 0, std::bit_cast<u32>(scale_y), 0, std::bit_cast<u32>(offset_y));
		utils::stream_vector(static_cast<char*>(buffer) + 32, 0, 0, std::bit_cast<u32>(scale_z), std::bit_cast<u32>(offset_z));
		utils::stream_vector(static_cast<char*>(buffer) + 48, 0, 0, 0, std::bit_cast<u32>(one));
	}

	void draw_command_processor::fill_user_clip_data(void* buffer) const
	{
		const rsx::user_clip_plane_op clip_plane_control[6] =
		{
			REGS(m_ctx)->clip_plane_0_enabled(),
			REGS(m_ctx)->clip_plane_1_enabled(),
			REGS(m_ctx)->clip_plane_2_enabled(),
			REGS(m_ctx)->clip_plane_3_enabled(),
			REGS(m_ctx)->clip_plane_4_enabled(),
			REGS(m_ctx)->clip_plane_5_enabled(),
		};

		u8 data_block[64];
		s32* clip_enabled_flags = reinterpret_cast<s32*>(data_block);
		f32* clip_distance_factors = reinterpret_cast<f32*>(data_block + 32);

		for (int index = 0; index < 6; ++index)
		{
			switch (clip_plane_control[index])
			{
			default:
				rsx_log.error("bad clip plane control (0x%x)", static_cast<u8>(clip_plane_control[index]));
				[[fallthrough]];

			case rsx::user_clip_plane_op::disable:
				clip_enabled_flags[index] = 0;
				clip_distance_factors[index] = 0.f;
				break;

			case rsx::user_clip_plane_op::greater_or_equal:
				clip_enabled_flags[index] = 1;
				clip_distance_factors[index] = 1.f;
				break;

			case rsx::user_clip_plane_op::less_than:
				clip_enabled_flags[index] = 1;
				clip_distance_factors[index] = -1.f;
				break;
			}
		}

		memcpy(buffer, data_block, 2 * 8 * sizeof(u32));
	}

	/**
	* Fill buffer with vertex program constants.
	* Buffer must be at least 512 float4 wide.
	*/
	void draw_command_processor::fill_vertex_program_constants_data(void* buffer, const std::span<const u16>& reloc_table) const
	{
		if (!reloc_table.empty()) [[ likely ]]
		{
			char* dst = reinterpret_cast<char*>(buffer);
			for (const auto& index : reloc_table)
			{
				utils::stream_vector_from_memory(dst, &REGS(m_ctx)->transform_constants[index]);
				dst += 16;
			}
		}
		else
		{
			memcpy(buffer, REGS(m_ctx)->transform_constants.data(), 468 * 4 * sizeof(float));
		}
	}

	void draw_command_processor::fill_fragment_state_buffer(void* buffer, const RSXFragmentProgram& /*fragment_program*/) const
	{
		ROP_control_t rop_control{};

		if (REGS(m_ctx)->alpha_test_enabled())
		{
			const u32 alpha_func = static_cast<u32>(REGS(m_ctx)->alpha_func());
			rop_control.set_alpha_test_func(alpha_func);
			rop_control.enable_alpha_test();
		}

		if (REGS(m_ctx)->polygon_stipple_enabled())
		{
			rop_control.enable_polygon_stipple();
		}

		auto can_use_hw_a2c = [&]() -> bool
		{
			const auto& config = RSX(m_ctx)->get_backend_config();
			if (!config.supports_hw_a2c)
			{
				return false;
			}

			if (config.supports_hw_a2c_1spp)
			{
				return true;
			}

			return REGS(m_ctx)->surface_antialias() != rsx::surface_antialiasing::center_1_sample;
		};

		if (REGS(m_ctx)->msaa_alpha_to_coverage_enabled() && !can_use_hw_a2c())
		{
			// TODO: Properly support alpha-to-coverage and alpha-to-one behavior in shaders
			// Alpha values generate a coverage mask for order independent blending
			// Requires hardware AA to work properly (or just fragment sample stage in fragment shaders)
			// Simulated using combined alpha blend and alpha test
			rop_control.enable_alpha_to_coverage();
			if (REGS(m_ctx)->msaa_sample_mask())
			{
				rop_control.enable_MSAA_writes();
			}

			// Sample configuration bits
			switch (REGS(m_ctx)->surface_antialias())
			{
			case rsx::surface_antialiasing::center_1_sample:
				break;
			case rsx::surface_antialiasing::diagonal_centered_2_samples:
				rop_control.set_msaa_control(1u);
				break;
			default:
				rop_control.set_msaa_control(3u);
				break;
			}
		}

		const f32 fog0 = REGS(m_ctx)->fog_params_0();
		const f32 fog1 = REGS(m_ctx)->fog_params_1();
		const u32 fog_mode = static_cast<u32>(REGS(m_ctx)->fog_equation());

		// Check if framebuffer is actually an XRGB format and not a WZYX format
		switch (REGS(m_ctx)->surface_color())
		{
		case rsx::surface_color_format::w16z16y16x16:
		case rsx::surface_color_format::w32z32y32x32:
		case rsx::surface_color_format::x32:
			// These behave very differently from "normal" formats.
			break;
		default:
			// Integer framebuffer formats.
			rop_control.enable_framebuffer_INT();

			// Check if we want sRGB conversion.
			if (REGS(m_ctx)->framebuffer_srgb_enabled())
			{
				rop_control.enable_framebuffer_sRGB();
			}
			break;
		}

		// Generate wpos coefficients
		// wpos equation is now as follows:
		// wpos.y = (frag_coord / resolution_scale) * ((window_origin!=top)?-1.: 1.) + ((window_origin!=top)? window_height : 0)
		// wpos.x = (frag_coord / resolution_scale)
		// wpos.zw = frag_coord.zw

		const auto window_origin = REGS(m_ctx)->shader_window_origin();
		const u32 window_height = REGS(m_ctx)->shader_window_height();
		const f32 resolution_scale = (window_height <= static_cast<u32>(g_cfg.video.min_scalable_dimension)) ? 1.f : rsx::get_resolution_scale();
		const f32 wpos_scale = (window_origin == rsx::window_origin::top) ? (1.f / resolution_scale) : (-1.f / resolution_scale);
		const f32 wpos_bias = (window_origin == rsx::window_origin::top) ? 0.f : window_height;
		const f32 alpha_ref = REGS(m_ctx)->alpha_ref();

		u32* dst = static_cast<u32*>(buffer);
		utils::stream_vector(dst, std::bit_cast<u32>(fog0), std::bit_cast<u32>(fog1), rop_control.value, std::bit_cast<u32>(alpha_ref));
		utils::stream_vector(dst + 4, 0u, fog_mode, std::bit_cast<u32>(wpos_scale), std::bit_cast<u32>(wpos_bias));
	}

	void draw_command_processor::fill_constants_instancing_buffer(rsx::io_buffer& indirection_table_buf, rsx::io_buffer& constants_data_array_buffer, const VertexProgramBase* prog) const
	{
		auto& draw_call = REGS(m_ctx)->current_draw_clause;

		// Only call this for instanced draws!
		ensure(draw_call.is_trivial_instanced_draw);

		// Temp indirection table. Used to track "running" updates.
		auto& instancing_indirection_table = m_scratch_buffers.u32buf;

		// indirection table size
		const auto full_reupload = !prog || prog->has_indexed_constants;
		const auto reloc_table = full_reupload ? decltype(prog->constant_ids){} : prog->constant_ids;
		const auto redirection_table_size = full_reupload ? 468u : ::size32(prog->constant_ids);
		instancing_indirection_table.resize(redirection_table_size);

		// Temp constants data
		auto& constants_data = m_scratch_buffers.u128buf;
		constants_data.clear();
		constants_data.reserve(redirection_table_size * draw_call.pass_count());

		// Allocate indirection buffer on GPU stream
		indirection_table_buf.reserve(instancing_indirection_table.size_bytes() * draw_call.pass_count());
		auto indirection_out = indirection_table_buf.data<u32>();

		rsx::instanced_draw_config_t instance_config;
		u32 indirection_table_offset = 0;

		// We now replay the draw call here to pack the data.
		draw_call.begin();

		// Write initial draw data.
		std::iota(instancing_indirection_table.begin(), instancing_indirection_table.end(), 0);

		constants_data.resize(redirection_table_size);
		fill_vertex_program_constants_data(constants_data.data(), reloc_table);

		// Next draw. We're guaranteed more than one draw call by the caller.
		draw_call.next();

		do
		{
			// Write previous state
			std::memcpy(indirection_out + indirection_table_offset, instancing_indirection_table.data(), instancing_indirection_table.size_bytes());
			indirection_table_offset += redirection_table_size;

			// Decode next draw state
			instance_config = {};
			draw_call.execute_pipeline_dependencies(m_ctx, &instance_config);

			if (!instance_config.transform_constants_data_changed)
			{
				continue;
			}

			const int translated_offset = full_reupload
				? instance_config.patch_load_offset
				: prog->translate_constants_range(instance_config.patch_load_offset, instance_config.patch_load_count);

			if (translated_offset >= 0)
			{
				// Trivially patchable in bulk
				const u32 redirection_loc = ::size32(constants_data);
				constants_data.resize(::size32(constants_data) + instance_config.patch_load_count);
				std::memcpy(constants_data.data() + redirection_loc, &REGS(m_ctx)->transform_constants[instance_config.patch_load_offset], instance_config.patch_load_count * sizeof(u128));

				// Update indirection table
				for (auto i = translated_offset, count = 0;
					 static_cast<u32>(count) < instance_config.patch_load_count;
					 ++i, ++count)
				{
					instancing_indirection_table[i] = redirection_loc + count;
				}

				continue;
			}

			ensure(!full_reupload);

			// Sparse update. Update records individually instead of bulk
			// FIXME: Range batching optimization
			const auto load_end = instance_config.patch_load_offset + instance_config.patch_load_count;
			for (u32 i = 0; i < redirection_table_size; ++i)
			{
				const auto read_index = prog->constant_ids[i];
				if (read_index < instance_config.patch_load_offset || read_index >= load_end)
				{
					// Reading outside "hot" range.
					continue;
				}

				const u32 redirection_loc = ::size32(constants_data);
				constants_data.resize(::size32(constants_data) + 1);
				std::memcpy(constants_data.data() + redirection_loc, &REGS(m_ctx)->transform_constants[read_index], sizeof(u128));

				instancing_indirection_table[i] = redirection_loc;
			}

		} while (draw_call.next());

		// Tail
		ensure(indirection_table_offset < (instancing_indirection_table.size() * draw_call.pass_count()));
		std::memcpy(indirection_out + indirection_table_offset, instancing_indirection_table.data(), instancing_indirection_table.size_bytes());

		// Now write the constants to the GPU buffer
		constants_data_array_buffer.reserve(constants_data.size_bytes());
		std::memcpy(constants_data_array_buffer.data(), constants_data.data(), constants_data.size_bytes());
	}
}
