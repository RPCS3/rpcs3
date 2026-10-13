#include "stdafx.h"
#include "RSXGraphEngine.h"

#include "Emu/RSX/Common/surface_store.h"
#include "Emu/RSX/Common/TextureUtils.h"
#include "Emu/RSX/Program/ProgramStateCache.h"
#include "Emu/RSX/Program/RSXFragmentProgram.h"
#include "Emu/RSX/Program/RSXVertexProgram.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/rsx_methods.h"
#include "Emu/system_config.h"

#include "Emu/RSX/NV47/HW/context_accessors.define.h"

namespace rsx
{
	constexpr u32 fs_export_config_mask =
		RSX_SHADER_CONTROL_EMULATE_DEPTH_COMPARE |
		RSX_SHADER_CONTROL_ROP_MULTISAMPLED |
		RSX_SHADER_CONTROL_PROGRAMMABLE_BLENDING |
		RSX_SHADER_CONTROL_EMULATE_DEPTH_RANGE;

	static bool evaluate_programmable_blending_state(rsx::context* ctx, u32& fragment_ctrl)
	{
		const bool is_blend_config_dirty = ctx->graphics_state.test(rsx::blend_config_dirty);
		ctx->graphics_state.clear(rsx::blend_config_dirty);

		const auto blend_enable_mask = REGS(ctx)->blend_enabled_mask() & REGS(ctx)->surface_color_target_mask();
		const bool programmable_blend_active = !!(fragment_ctrl & RSX_SHADER_CONTROL_PROGRAMMABLE_BLENDING);
		const bool is_blending_active = !!blend_enable_mask && !REGS(ctx)->logic_op_enabled();

		if (!is_blending_active && !programmable_blend_active)
		{
			return false;
		}

		if (!is_blending_active)
		{
			fragment_ctrl &= ~RSX_SHADER_CONTROL_PROGRAMMABLE_BLENDING;
			return true;
		}

		if (g_cfg.video.disable_hardware_blending)
		{
			fragment_ctrl |= RSX_SHADER_CONTROL_PROGRAMMABLE_BLENDING;
			return !programmable_blend_active;
		}

		// We actually need to handle this, we're still blending
		const auto is_signed_equation = [](rsx::blend_equation equation)
		{
			switch (equation)
			{
			case rsx::blend_equation::add_signed:
			case rsx::blend_equation::reverse_add_signed:
			case rsx::blend_equation::reverse_subtract_signed:
				return true;
			default:
				return false;
			}
		};

		const auto factor_references_alpha = [](rsx::blend_factor factor)
		{
			switch (factor)
			{
			case rsx::blend_factor::src_alpha:
			case rsx::blend_factor::one_minus_src_alpha:
			case rsx::blend_factor::dst_alpha:
			case rsx::blend_factor::one_minus_dst_alpha:
			case rsx::blend_factor::src_alpha_saturate:
				return true;
			default:
				return false;
			}
		};

		const auto shader_writes_alpha = [&]()
		{
			for (u32 i = 0, mask = blend_enable_mask; !!mask; mask >>= 1, i++)
			{
				if (REGS(ctx)->color_mask_a(i))
				{
					return true;
				}
			}
			return false;
		};

		bool need_programmable_blending = false;
		const auto surface_format = REGS(ctx)->surface_color();

		switch (surface_format)
		{
		case surface_color_format::x1r5g5b5_z1r5g5b5:
		case surface_color_format::x1r5g5b5_o1r5g5b5:
		case surface_color_format::x8r8g8b8_z8r8g8b8:
		case surface_color_format::x8r8g8b8_o8r8g8b8:
		case surface_color_format::x8b8g8r8_z8b8g8r8:
		case surface_color_format::x8b8g8r8_o8b8g8r8:
			// Force PB if alpha is read in the RGB factors or A is written without passthrough.
			if (factor_references_alpha(REGS(ctx)->blend_func_sfactor_rgb()) ||
				factor_references_alpha(REGS(ctx)->blend_func_dfactor_rgb()))
			{
				need_programmable_blending = true;
			}
			else
			{
				need_programmable_blending = shader_writes_alpha() && (
					REGS(ctx)->blend_equation_a() == rsx::blend_equation::reverse_subtract ||
					REGS(ctx)->blend_equation_a() == rsx::blend_equation::min ||
					REGS(ctx)->blend_equation_a() == rsx::blend_equation::max ||
					REGS(ctx)->blend_func_sfactor_a() != rsx::blend_factor::one ||
					REGS(ctx)->blend_func_dfactor_a() != rsx::blend_factor::zero);
			}
			break;
		case surface_color_format::b8:
		case surface_color_format::g8b8:
			// These 2 don't accept custom factors anyway
			[[ fallthrough ]];
		default:
			need_programmable_blending = need_programmable_blending ||
				is_signed_equation(REGS(ctx)->blend_equation_rgb()) ||
				is_signed_equation(REGS(ctx)->blend_equation_a());
			break;
		}

		if (need_programmable_blending && is_blend_config_dirty)
		{
			ctx->graphics_state.set(rsx::fragment_state_dirty);
		}

		if (need_programmable_blending == programmable_blend_active)
		{
			return false;
		}

		if (!need_programmable_blending)
		{
			fragment_ctrl &= ~RSX_SHADER_CONTROL_PROGRAMMABLE_BLENDING;
			return true;
		}

		fragment_ctrl |= RSX_SHADER_CONTROL_PROGRAMMABLE_BLENDING;
		return true;
	}

	std::array<u32, 4> graph_engine::get_color_surface_addresses() const
	{
		u32 offset_color[] =
		{
			REGS(m_ctx)->surface_offset(0),
			REGS(m_ctx)->surface_offset(1),
			REGS(m_ctx)->surface_offset(2),
			REGS(m_ctx)->surface_offset(3),
		};
		u32 context_dma_color[] =
		{
			REGS(m_ctx)->surface_dma(0),
			REGS(m_ctx)->surface_dma(1),
			REGS(m_ctx)->surface_dma(2),
			REGS(m_ctx)->surface_dma(3),
		};
		return
		{
			rsx::get_address(offset_color[0], context_dma_color[0]),
			rsx::get_address(offset_color[1], context_dma_color[1]),
			rsx::get_address(offset_color[2], context_dma_color[2]),
			rsx::get_address(offset_color[3], context_dma_color[3]),
		};
	}

	u32 graph_engine::get_zeta_surface_address() const
	{
		u32 m_context_dma_z = REGS(m_ctx)->surface_z_dma();
		u32 offset_zeta = REGS(m_ctx)->surface_z_offset();
		return rsx::get_address(offset_zeta, m_context_dma_z);
	}

	void graph_engine::get_framebuffer_layout(rsx::framebuffer_creation_context context, framebuffer_layout &layout)
	{
		layout = {};

		layout.ignore_change = true;
		layout.width = REGS(m_ctx)->surface_clip_width();
		layout.height = REGS(m_ctx)->surface_clip_height();

		// NOTE: rtt_config_no_attachments is intentionally not reset.
		m_ctx->graphics_state.clear(rsx::rtt_config_contested | rsx::rtt_config_valid);
		m_ctx->fb.current_context = context;

		if (layout.width == 0 || layout.height == 0)
		{
			rsx_log.trace("Invalid framebuffer setup, w=%d, h=%d", layout.width, layout.height);
			return;
		}

		//const u16 clip_x = REGS(m_ctx)->surface_clip_origin_x();
		//const u16 clip_y = REGS(m_ctx)->surface_clip_origin_y();

		layout.color_addresses = get_color_surface_addresses();
		layout.zeta_address = get_zeta_surface_address();
		layout.zeta_pitch = REGS(m_ctx)->surface_z_pitch();
		layout.color_pitch =
		{
			REGS(m_ctx)->surface_pitch(0),
			REGS(m_ctx)->surface_pitch(1),
			REGS(m_ctx)->surface_pitch(2),
			REGS(m_ctx)->surface_pitch(3),
		};

		layout.color_format = REGS(m_ctx)->surface_color();
		layout.depth_format = REGS(m_ctx)->surface_depth_fmt();
		layout.target = REGS(m_ctx)->surface_color_target();

		const auto mrt_buffers = rsx::utility::get_rtt_indexes(layout.target);
		const auto aa_mode = REGS(m_ctx)->surface_antialias();
		const u32 aa_factor_u = (aa_mode == rsx::surface_antialiasing::center_1_sample) ? 1 : 2;
		const u32 aa_factor_v = (aa_mode == rsx::surface_antialiasing::center_1_sample || aa_mode == rsx::surface_antialiasing::diagonal_centered_2_samples) ? 1 : 2;
		const u8 sample_count = get_format_sample_count(aa_mode);

		const auto depth_texel_size = get_format_block_size_in_bytes(layout.depth_format) * aa_factor_u;
		const auto color_texel_size = get_format_block_size_in_bytes(layout.color_format) * aa_factor_u;
		const bool stencil_test_enabled = is_depth_stencil_format(layout.depth_format) && REGS(m_ctx)->stencil_test_enabled();
		const bool depth_test_enabled = REGS(m_ctx)->depth_test_enabled();

		// Check write masks
		layout.zeta_write_enabled = (depth_test_enabled && REGS(m_ctx)->depth_write_enabled());
		if (!layout.zeta_write_enabled && stencil_test_enabled)
		{
			// Check if stencil data is modified
			auto mask = REGS(m_ctx)->stencil_mask();
			bool active_write_op = (REGS(m_ctx)->stencil_op_zpass() != rsx::stencil_op::keep ||
				REGS(m_ctx)->stencil_op_fail() != rsx::stencil_op::keep ||
				REGS(m_ctx)->stencil_op_zfail() != rsx::stencil_op::keep);

			if ((!mask || !active_write_op) && REGS(m_ctx)->two_sided_stencil_test_enabled())
			{
				mask |= REGS(m_ctx)->back_stencil_mask();
				active_write_op |= (REGS(m_ctx)->back_stencil_op_zpass() != rsx::stencil_op::keep ||
					REGS(m_ctx)->back_stencil_op_fail() != rsx::stencil_op::keep ||
					REGS(m_ctx)->back_stencil_op_zfail() != rsx::stencil_op::keep);
			}

			layout.zeta_write_enabled = (mask && active_write_op);
		}

		// NOTE: surface_target_a is index 1 but is not MRT since only one surface is active
		bool color_write_enabled = false;
		for (uint i = 0; i < mrt_buffers.size(); ++i)
		{
			if (REGS(m_ctx)->color_write_enabled(i))
			{
				const auto real_index = mrt_buffers[i];
				layout.color_write_enabled[real_index] = true;
				color_write_enabled = true;
			}
		}

		bool depth_buffer_unused = false, color_buffer_unused = false;

		switch (context)
		{
		case rsx::framebuffer_creation_context::context_clear_all:
			break;
		case rsx::framebuffer_creation_context::context_clear_depth:
			color_buffer_unused = true;
			break;
		case rsx::framebuffer_creation_context::context_clear_color:
			depth_buffer_unused = true;
			break;
		case rsx::framebuffer_creation_context::context_draw:
			// NOTE: As with all other hw, depth/stencil writes involve the corresponding depth/stencil test, i.e No test = No write
			// NOTE: Depth test is not really using the memory if its set to always or never
			// TODO: Perform similar checks for stencil test
			if (!stencil_test_enabled)
			{
				if (!depth_test_enabled)
				{
					depth_buffer_unused = true;
				}
				else if (!REGS(m_ctx)->depth_write_enabled())
				{
					// Depth test is enabled but depth write is disabled
					switch (REGS(m_ctx)->depth_func())
					{
					default:
						break;
					case rsx::comparison_function::never:
					case rsx::comparison_function::always:
						// No access to depth buffer memory
						depth_buffer_unused = true;
						break;
					}
				}

				if (depth_buffer_unused) [[unlikely]]
				{
					// Check if depth bounds is active. Depth bounds test does NOT need depth test to be enabled to access the Z buffer
					// Bind Z buffer in read mode for bounds check in this case
					if (REGS(m_ctx)->depth_bounds_test_enabled() &&
						(REGS(m_ctx)->depth_bounds_min() > 0.f || REGS(m_ctx)->depth_bounds_max() < 1.f))
					{
						depth_buffer_unused = false;
					}
				}
			}

			color_buffer_unused = !color_write_enabled || layout.target == rsx::surface_target::none;
			if (color_buffer_unused || depth_buffer_unused)
			{
				m_ctx->graphics_state.set(rsx::rtt_config_contested);
			}
			break;
		default:
			fmt::throw_exception("Unknown framebuffer context 0x%x", static_cast<u32>(context));
		}

		// Swizzled render does tight packing of bytes
		bool packed_render = false;
		u32 minimum_color_pitch = 64u;
		u32 minimum_zeta_pitch = 64u;

		switch (layout.raster_type = REGS(m_ctx)->surface_type())
		{
		default:
			rsx_log.error("Unknown raster mode 0x%x", static_cast<u32>(layout.raster_type));
			[[fallthrough]];
		case rsx::surface_raster_type::linear:
			break;
		case rsx::surface_raster_type::swizzle:
			packed_render = true;
			break;
		}

		if (!packed_render)
		{
			// Well, this is a write operation either way (clearing or drawing)
			// We can deduce a minimum pitch for which this operation is guaranteed to require by checking for the lesser of scissor or clip
			const u32 write_limit_x = std::min<u32>(layout.width, REGS(m_ctx)->scissor_origin_x() + REGS(m_ctx)->scissor_width());

			minimum_color_pitch = color_texel_size * write_limit_x;
			minimum_zeta_pitch = depth_texel_size * write_limit_x;

			// Check for size fit and attempt to correct incorrect inputs.
			// BLUS30072 is misconfigured here and renders fine on PS3. The width fails to account for AA being active in that engine.
			u16 corrected_width = umax;
			std::vector<u32*> pitch_fixups;

			if (!depth_buffer_unused)
			{
				if (layout.zeta_pitch < minimum_zeta_pitch)
				{
					// Observed in CoD3 where the depth buffer is clearly misconfigured.
					if (layout.zeta_pitch > 64)
					{
						corrected_width = layout.zeta_pitch / depth_texel_size;
						layout.zeta_pitch = depth_texel_size;
						pitch_fixups.push_back(&layout.zeta_pitch);
					}
					else
					{
						rsx_log.warning("Misconfigured surface could not fit a depth buffer. Dropping.");
						layout.zeta_address = 0;
					}
				}
				else if (layout.width * depth_texel_size > layout.zeta_pitch)
				{
					// This is ok, misconfigured raster dimensions, but we're only writing the pitch as determined by the scissor
					corrected_width = layout.zeta_pitch / depth_texel_size;
				}
			}

			if (!color_buffer_unused)
			{
				for (const auto& index : rsx::utility::get_rtt_indexes(layout.target))
				{
					if (layout.color_pitch[index] < minimum_color_pitch)
					{
						if (layout.color_pitch[index] > 64)
						{
							corrected_width = std::min<u16>(corrected_width, layout.color_pitch[index] / color_texel_size);
							layout.color_pitch[index] = color_texel_size;
							pitch_fixups.push_back(&layout.color_pitch[index]);
						}
						else
						{
							rsx_log.warning("Misconfigured surface could not fit color buffer %d. Dropping.", index);
							layout.color_addresses[index] = 0;
						}

						continue;
					}

					if (layout.width * color_texel_size > layout.color_pitch[index])
					{
						// This is ok, misconfigured raster dimensions, but we're only writing the pitch as determined by the scissor
						corrected_width = std::min<u16>(corrected_width, layout.color_pitch[index] / color_texel_size);
					}
				}
			}

			if (corrected_width != umax)
			{
				layout.width = corrected_width;

				for (auto& value : pitch_fixups)
				{
					*value = *value * layout.width;
				}
			}
		}

		if (depth_buffer_unused)
		{
			layout.zeta_address = 0;
		}
		else if (packed_render)
		{
			layout.actual_zeta_pitch = (layout.width * depth_texel_size);
		}
		else
		{
			layout.actual_zeta_pitch = layout.zeta_pitch;
		}

		for (const auto &index : rsx::utility::get_rtt_indexes(layout.target))
		{
			if (color_buffer_unused)
			{
				layout.color_addresses[index] = 0;
				continue;
			}

			if (layout.color_pitch[index] < minimum_color_pitch)
			{
				// Unlike the depth buffer, when given a color target we know it is intended to be rendered to
				rsx_log.warning("Framebuffer setup error: Color target failed pitch check, Pitch=[%d, %d, %d, %d] + %d, target=%d, context=%d",
					layout.color_pitch[0], layout.color_pitch[1], layout.color_pitch[2], layout.color_pitch[3],
					layout.zeta_pitch, static_cast<u32>(layout.target), static_cast<u32>(context));

				// Some games (COD4) are buggy and set incorrect width + AA + pitch combo. Force fit in such scenarios.
				if (layout.color_pitch[index] > 64)
				{
					layout.width = layout.color_pitch[index] / color_texel_size;
				}
				else
				{
					layout.color_addresses[index] = 0;
					continue;
				}
			}

			if (layout.color_addresses[index] == layout.zeta_address)
			{
				rsx_log.warning("Framebuffer at 0x%X has aliasing color/depth targets, color_index=%d, zeta_pitch = %d, color_pitch=%d, context=%d",
					layout.zeta_address, index, layout.zeta_pitch, layout.color_pitch[index], static_cast<u32>(context));

				m_ctx->graphics_state.set(rsx::rtt_config_contested);

				if (g_cfg.video.fb_aliasing_bias == framebuffer_aliasing_bias::prefer_color
					&& layout.color_write_enabled[index]
					&& !layout.zeta_write_enabled)
				{
					// Use address for color data
					layout.zeta_address = 0;
				}
				else
				{
					// TODO: Research clearing both depth AND color
					// TODO: If context is creation_draw, deal with possibility of a lost buffer clear
					if (depth_test_enabled || stencil_test_enabled || (!layout.color_write_enabled[index] && layout.zeta_write_enabled))
					{
						// Use address for depth data
						layout.color_addresses[index] = 0;
						continue;
					}
					else
					{
						// Use address for color data
						layout.zeta_address = 0;
					}
				}
			}

			ensure(layout.color_addresses[index]);

			const auto packed_pitch = (layout.width * color_texel_size);
			if (packed_render)
			{
				layout.actual_color_pitch[index] = packed_pitch;
			}
			else
			{
				layout.actual_color_pitch[index] = layout.color_pitch[index];
			}

			m_ctx->graphics_state.set(rsx::rtt_config_valid);
		}

		const bool framebufferless = !m_ctx->graphics_state.test(rsx::rtt_config_valid) && !layout.zeta_address;
		if (framebufferless)
		{
			// No attachments. The hardware still rasterizes in this case and side effects such as ZPASS counters are still updated.
			if (context != rsx::framebuffer_creation_context::context_draw ||
				!RSX(m_ctx)->get_backend_config().supports_framebufferless_rendering)
			{
				rsx_log.warning("Framebuffer setup failed. Draw calls may have been lost");
				return;
			}

			// Context must be draw now...
		}

		if (framebufferless != m_ctx->graphics_state.test(rsx::rtt_config_no_attachments))
		{
			// Programmable blending reads back from the color attachments and is disabled when there are none.
			// Force the blend configuration to be re-evaluated when moving in or out of this state.
			if (framebufferless)
			{
				m_ctx->graphics_state.set(rsx::rtt_config_no_attachments);
			}
			else
			{
				m_ctx->graphics_state.clear(rsx::rtt_config_no_attachments);
			}

			m_ctx->graphics_state.set(rsx::pipeline_config_dirty);
		}

		// At least one attachment exists
		m_ctx->graphics_state.set(rsx::rtt_config_valid);

		// Window (raster) offsets
		const auto window_offset_x = REGS(m_ctx)->window_offset_x();
		const auto window_offset_y = REGS(m_ctx)->window_offset_y();
		const auto window_clip_width = REGS(m_ctx)->window_clip_horizontal();
		const auto window_clip_height = REGS(m_ctx)->window_clip_vertical();

		if (window_offset_x || window_offset_y)
		{
			// Window offset is what affects the raster position!
			// Tested with Turbo: Super stunt squad that only changes the window offset to declare new framebuffers
			// Sampling behavior clearly indicates the addresses are expected to have changed
			if (auto clip_type = REGS(m_ctx)->window_clip_type())
				rsx_log.error("Unknown window clip type 0x%X", clip_type);

			for (const auto &index : rsx::utility::get_rtt_indexes(layout.target))
			{
				if (layout.color_addresses[index])
				{
					const u32 window_offset_bytes = (layout.actual_color_pitch[index] * window_offset_y) + (color_texel_size * window_offset_x);
					layout.color_addresses[index] += window_offset_bytes;
				}
			}

			if (layout.zeta_address)
			{
				layout.zeta_address += (layout.actual_zeta_pitch * window_offset_y) + (depth_texel_size * window_offset_x);
			}
		}

		if ((window_clip_width && window_clip_width < layout.width) ||
			(window_clip_height && window_clip_height < layout.height))
		{
			rsx_log.error("Unexpected window clip dimensions: window_clip=%dx%d, surface_clip=%dx%d",
				window_clip_width, window_clip_height, layout.width, layout.height);
		}

		layout.aa_mode = aa_mode;
		layout.aa_factors[0] = aa_factor_u;
		layout.aa_factors[1] = aa_factor_v;

		// Log this to frame stats
		if (layout.target != rsx::surface_target::none)
		{
			m_ctx->stats.framebuffer_stats.add(layout.width, layout.height, aa_mode);
		}

		// Check if anything has changed
		bool really_changed = false;

		for (u8 i = 0; i < rsx::limits::color_buffers_count; ++i)
		{
			if (m_ctx->fb.surface_info[i].address != layout.color_addresses[i])
			{
				really_changed = true;
				break;
			}

			if (layout.color_addresses[i])
			{
				if (m_ctx->fb.surface_info[i].width != layout.width ||
					m_ctx->fb.surface_info[i].height != layout.height ||
					m_ctx->fb.surface_info[i].color_format != layout.color_format ||
					m_ctx->fb.surface_info[i].samples != sample_count)
				{
					really_changed = true;
					break;
				}
			}
		}

		// Framebufferless setups have no surface info to compare against. Always rebuild.
		if (!really_changed && !framebufferless)
		{
			if (layout.zeta_address == m_ctx->fb.depth_surface_info.address &&
				layout.depth_format == m_ctx->fb.depth_surface_info.depth_format &&
				sample_count == m_ctx->fb.depth_surface_info.samples &&
				(!layout.zeta_address || (m_ctx->fb.depth_surface_info.width == layout.width && m_ctx->fb.depth_surface_info.height == layout.height)))
			{
				// Same target is reused
				return;
			}
		}

		layout.ignore_change = false;
	}

	void graph_engine::on_framebuffer_options_changed(u32 opt)
	{
		if (m_ctx->graphics_state & rsx::rtt_config_dirty)
		{
			// Nothing to do
			return;
		}

		auto set_zeta_write_enabled = [&](bool state)
		{
			if (state == m_ctx->fb.layout.zeta_write_enabled)
			{
				return;
			}

			if (m_ctx->graphics_state & rsx::zeta_address_is_cyclic)
			{
				m_ctx->graphics_state |= rsx::fragment_program_state_dirty;
			}
			m_ctx->fb.layout.zeta_write_enabled = state;
		};

		auto evaluate_depth_buffer_state = [&]()
		{
			const bool zeta_write_en = (REGS(m_ctx)->depth_test_enabled() && REGS(m_ctx)->depth_write_enabled());
			set_zeta_write_enabled(zeta_write_en);
		};

		auto evaluate_stencil_buffer_state = [&]()
		{
			if (!m_ctx->fb.layout.zeta_write_enabled &&
				REGS(m_ctx)->stencil_test_enabled() &&
				is_depth_stencil_format(m_ctx->fb.layout.depth_format))
			{
				// Check if stencil data is modified
				auto mask = REGS(m_ctx)->stencil_mask();
				bool active_write_op = (REGS(m_ctx)->stencil_op_zpass() != rsx::stencil_op::keep ||
					REGS(m_ctx)->stencil_op_fail() != rsx::stencil_op::keep ||
					REGS(m_ctx)->stencil_op_zfail() != rsx::stencil_op::keep);

				if ((!mask || !active_write_op) && REGS(m_ctx)->two_sided_stencil_test_enabled())
				{
					mask |= REGS(m_ctx)->back_stencil_mask();
					active_write_op |= (REGS(m_ctx)->back_stencil_op_zpass() != rsx::stencil_op::keep ||
						REGS(m_ctx)->back_stencil_op_fail() != rsx::stencil_op::keep ||
						REGS(m_ctx)->back_stencil_op_zfail() != rsx::stencil_op::keep);
				}

				set_zeta_write_enabled(mask && active_write_op);
			}
		};

		auto evaluate_color_buffer_state = [&]() -> bool
		{
			m_ctx->fb.layout.color_write_enabled = {};
			const auto mrt_buffers = rsx::utility::get_rtt_indexes(m_ctx->fb.layout.target);
			bool any_found = false;

			for (uint i = 0; i < mrt_buffers.size(); ++i)
			{
				if (REGS(m_ctx)->color_write_enabled(i))
				{
					const auto real_index = mrt_buffers[i];
					m_ctx->fb.layout.color_write_enabled[real_index] = true;
					any_found = true;
				}
			}

			on_framebuffer_layout_updated();
			return any_found;
		};

		auto evaluate_depth_buffer_contested = [&]()
		{
			if (m_ctx->fb.layout.zeta_address) [[likely]]
			{
				// Nothing to do, depth buffer already exists
				return false;
			}

			// Check if depth read/write is enabled
			if (m_ctx->fb.layout.zeta_write_enabled ||
				REGS(m_ctx)->depth_test_enabled())
			{
				return true;
			}

			// Check if stencil read is enabled
			if (is_depth_stencil_format(m_ctx->fb.layout.depth_format) &&
				REGS(m_ctx)->stencil_test_enabled())
			{
				return true;
			}

			return false;
		};

		switch (opt)
		{
		case NV4097_SET_DEPTH_TEST_ENABLE:
		case NV4097_SET_DEPTH_MASK:
		case NV4097_SET_DEPTH_FUNC:
		{
			evaluate_depth_buffer_state();

			if (m_ctx->graphics_state.test(rsx::rtt_config_contested) && evaluate_depth_buffer_contested())
			{
				m_ctx->graphics_state.set(rsx::rtt_config_dirty);
			}
			break;
		}
		case NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE:
		case NV4097_SET_STENCIL_TEST_ENABLE:
		case NV4097_SET_STENCIL_MASK:
		case NV4097_SET_STENCIL_OP_ZPASS:
		case NV4097_SET_STENCIL_OP_FAIL:
		case NV4097_SET_STENCIL_OP_ZFAIL:
		case NV4097_SET_BACK_STENCIL_MASK:
		case NV4097_SET_BACK_STENCIL_OP_ZPASS:
		case NV4097_SET_BACK_STENCIL_OP_FAIL:
		case NV4097_SET_BACK_STENCIL_OP_ZFAIL:
		{
			// Stencil takes a back seat to depth buffer stuff
			evaluate_depth_buffer_state();

			if (!m_ctx->fb.layout.zeta_write_enabled)
			{
				evaluate_stencil_buffer_state();
			}

			if (m_ctx->graphics_state.test(rsx::rtt_config_contested) && evaluate_depth_buffer_contested())
			{
				m_ctx->graphics_state.set(rsx::rtt_config_dirty);
			}
			break;
		}
		case NV4097_SET_COLOR_MASK:
		case NV4097_SET_COLOR_MASK_MRT:
		{
			if (!m_ctx->graphics_state.test(rsx::rtt_config_contested)) [[likely]]
			{
				// Update write masks and continue
				evaluate_color_buffer_state();
			}
			else
			{
				bool old_state = false;
				for (const auto& enabled : m_ctx->fb.layout.color_write_enabled)
				{
					if (old_state = enabled; old_state) break;
				}

				const auto new_state = evaluate_color_buffer_state();
				if (!old_state && new_state)
				{
					// Color buffers now in use
					m_ctx->graphics_state.set(rsx::rtt_config_dirty);
				}
			}
			break;
		}
		default:
			rsx_log.fatal("Unhandled framebuffer option changed 0x%x", opt);
			break;
		}
	}

	void graph_engine::on_framebuffer_layout_updated()
	{
		if (m_ctx->graphics_state.test(rsx::fragment_program_state_dirty))
		{
			return;
		}

		const auto target = REGS(m_ctx)->surface_color_target();
		if (rsx::utility::get_mrt_buffers_count(target) == m_ctx->program.fragment_program.mrt_buffers_count)
		{
			return;
		}

		m_ctx->graphics_state |= rsx::fragment_program_state_dirty;
	}

	void graph_engine::bind_framebuffer(rsx::framebuffer_creation_context context)
	{
		render_backend* backend = RSX(m_ctx)->renderer();
		const bool clipped_scissor = (context == rsx::framebuffer_creation_context::context_draw);

		if (m_ctx->fb.current_context == context && !m_ctx->graphics_state.test(rsx::rtt_config_dirty) && backend->is_framebuffer_bound())
		{
			// Fast path. Framebuffer usage has not changed, framebuffer exists and config regs have not changed
			resolve_scissor(clipped_scissor);
			return;
		}

		m_ctx->graphics_state.clear(
			rsx::rtt_config_dirty |
			rsx::rtt_config_contested |
			rsx::rtt_config_valid |
			rsx::rtt_cache_state_dirty);

		get_framebuffer_layout(context, m_ctx->fb.layout);
		if (!m_ctx->graphics_state.test(rsx::rtt_config_valid))
		{
			return;
		}

		if (m_ctx->fb.layout.ignore_change && backend->is_framebuffer_bound())
		{
			// Nothing has changed, we're still using the same framebuffer
			if (!backend->on_framebuffer_bound(m_ctx->fb.layout))
			{
				m_ctx->graphics_state.clear(rsx::rtt_config_valid);
			}

			resolve_scissor(clipped_scissor);
			return;
		}

		if (!backend->on_framebuffer_bound(m_ctx->fb.layout))
		{
			// The host could not build the framebuffer
			m_ctx->graphics_state.clear(rsx::rtt_config_valid);
			return;
		}

		resolve_scissor(clipped_scissor);
		on_framebuffer_layout_updated();

		// The backend has recorded the bound depth surface
		RSX(m_ctx)->check_zcull_status(true);
	}

	void graph_engine::resolve_scissor(bool clip_viewport)
	{
		if (!m_ctx->graphics_state.test(rsx::pipeline_state::scissor_config_state_dirty))
		{
			if (clip_viewport == m_ctx->graphics_state.test(rsx::pipeline_state::scissor_setup_clipped))
			{
				// Nothing to do
				return;
			}
		}

		m_ctx->graphics_state.clear(rsx::pipeline_state::scissor_config_state_dirty | rsx::pipeline_state::scissor_setup_clipped);

		u16 x1, x2, y1, y2;

		u16 scissor_x = REGS(m_ctx)->scissor_origin_x();
		u16 scissor_w = REGS(m_ctx)->scissor_width();
		u16 scissor_y = REGS(m_ctx)->scissor_origin_y();
		u16 scissor_h = REGS(m_ctx)->scissor_height();

		if (clip_viewport)
		{
			u16 raster_x = REGS(m_ctx)->viewport_origin_x();
			u16 raster_w = REGS(m_ctx)->viewport_width();
			u16 raster_y = REGS(m_ctx)->viewport_origin_y();
			u16 raster_h = REGS(m_ctx)->viewport_height();

			// Get the minimum area between these two
			x1 = std::max(scissor_x, raster_x);
			y1 = std::max(scissor_y, raster_y);
			x2 = std::min(scissor_x + scissor_w, raster_x + raster_w);
			y2 = std::min(scissor_y + scissor_h, raster_y + raster_h);

			m_ctx->graphics_state |= rsx::pipeline_state::scissor_setup_clipped;
		}
		else
		{
			x1 = scissor_x;
			x2 = scissor_x + scissor_w;
			y1 = scissor_y;
			y2 = scissor_y + scissor_h;
		}

		if (x2 <= x1 ||
			y2 <= y1 ||
			x1 >= REGS(m_ctx)->window_clip_horizontal() ||
			y1 >= REGS(m_ctx)->window_clip_vertical())
		{
			m_ctx->graphics_state |= rsx::pipeline_state::scissor_setup_invalid;
			m_ctx->graphics_state.clear(rsx::rtt_config_valid);
			return;
		}

		if (m_ctx->graphics_state & rsx::pipeline_state::scissor_setup_invalid)
		{
			m_ctx->graphics_state.clear(rsx::pipeline_state::scissor_setup_invalid);
			m_ctx->graphics_state.set(rsx::rtt_config_valid);
		}

		// The backend applies resolution scaling
		m_ctx->draw_state.scissor = { .x1 = x1, .y1 = y1, .x2 = x2, .y2 = y2 };
		m_ctx->draw_state.dirty.set(draw_state_t::group::scissor);
	}

	rsx::flags32_t graph_engine::get_fragment_program_export_config()
	{
		u32 expected_ctrl = 0;

		// Programmable blending. Requires color attachments to read from.
		if (RSX(m_ctx)->get_backend_config().supports_programmable_blending &&
			!m_ctx->graphics_state.test(rsx::rtt_config_no_attachments))
		{
			expected_ctrl = m_ctx->program.fragment_program.ctrl & RSX_SHADER_CONTROL_PROGRAMMABLE_BLENDING;

			if (m_ctx->graphics_state.test(rsx::pipeline_config_dirty))
			{
				evaluate_programmable_blending_state(m_ctx, expected_ctrl);
			}
		}

		// Resolve MSAA here if needed
		if (!!(expected_ctrl & RSX_SHADER_CONTROL_PROGRAMMABLE_BLENDING) &&
			REGS(m_ctx)->surface_antialias() != rsx::surface_antialiasing::center_1_sample &&
			RSX(m_ctx)->get_backend_config().supports_hw_msaa)
		{
			expected_ctrl |= RSX_SHADER_CONTROL_ROP_MULTISAMPLED;
		}

		if (requires_depth_range_emulation()) [[ unlikely ]]
		{
			expected_ctrl |= RSX_SHADER_CONTROL_EMULATE_DEPTH_RANGE;
		}

		// Depth compare
		if (!g_cfg.video.emulate_depth_compare) [[ likely ]]
		{
			return expected_ctrl;
		}

		if (REGS(m_ctx)->current_draw_clause.classify_mode() != primitive_class::polygon)
		{
			return expected_ctrl;
		}

		if (m_ctx->fb.layout.zeta_address &&
			REGS(m_ctx)->depth_test_enabled() &&
			REGS(m_ctx)->depth_func() == rsx::comparison_function::equal)
		{
			expected_ctrl |= RSX_SHADER_CONTROL_EMULATE_DEPTH_COMPARE;
		}

		// Resolve MSAA here if needed
		if ((expected_ctrl & (RSX_SHADER_CONTROL_EMULATE_DEPTH_COMPARE | RSX_SHADER_CONTROL_ROP_MULTISAMPLED)) == RSX_SHADER_CONTROL_EMULATE_DEPTH_COMPARE &&
			REGS(m_ctx)->surface_antialias() != rsx::surface_antialiasing::center_1_sample &&
			RSX(m_ctx)->get_backend_config().supports_hw_msaa)
		{
			expected_ctrl |= RSX_SHADER_CONTROL_ROP_MULTISAMPLED;
		}

		return expected_ctrl;
	}

	bool graph_engine::requires_depth_range_emulation() const
	{
		if (RSX(m_ctx)->get_backend_config().supports_extended_depth_range || !g_cfg.video.emulate_extended_depth_range) [[ likely ]]
		{
			return false;
		}

		return m_ctx->fb.layout.zeta_address && rsx::is_float_depth_format(REGS(m_ctx)->surface_depth_fmt());
	}

	void graph_engine::prefetch_fragment_program()
	{
		if (!m_ctx->graphics_state.test(rsx::pipeline_state::fragment_program_ucode_dirty))
		{
			return;
		}

		m_ctx->graphics_state.clear(rsx::pipeline_state::fragment_program_ucode_dirty);

		// Request for update of fragment constants if the program block is invalidated
		m_ctx->graphics_state |= rsx::pipeline_state::fragment_constants_dirty;

		const auto [program_offset, program_location] = REGS(m_ctx)->shader_program_address();
		const auto prev_textures_reference_mask = m_ctx->program.fp_metadata.referenced_textures_mask;

		auto data_ptr = vm::base(rsx::get_address(program_offset, program_location));
		m_ctx->program.fp_metadata = program_hash_util::fragment_program_utils::analyse_fragment_program(data_ptr);

		m_ctx->program.fragment_program.data = (static_cast<u8*>(data_ptr) + m_ctx->program.fp_metadata.program_start_offset);
		m_ctx->program.fragment_program.offset = program_offset + m_ctx->program.fp_metadata.program_start_offset;
		m_ctx->program.fragment_program.ucode_length = m_ctx->program.fp_metadata.program_ucode_length;
		m_ctx->program.fragment_program.total_length = m_ctx->program.fp_metadata.program_ucode_length + m_ctx->program.fp_metadata.program_start_offset;
		m_ctx->program.fragment_program.texture_state.import(m_ctx->program.fp_texture_state, m_ctx->program.fp_metadata.referenced_textures_mask);
		m_ctx->program.fragment_program.valid = true;

		// Verify current texture state is valid
		if (m_ctx->textures.fragment_textures_dirty & m_ctx->program.fp_metadata.referenced_textures_mask)
		{
			m_ctx->graphics_state |= rsx::pipeline_state::fragment_program_state_dirty;
		}

		if (!m_ctx->graphics_state.test(rsx::pipeline_state::fragment_program_state_dirty) &&
			(prev_textures_reference_mask != m_ctx->program.fp_metadata.referenced_textures_mask))
		{
			// If different textures are used, upload their coefficients.
			// The texture parameters transfer routine is optimized and only writes data for textures consumed by the ucode.
			m_ctx->graphics_state |= rsx::pipeline_state::fragment_texture_state_dirty;
		}
	}

	void graph_engine::prefetch_vertex_program()
	{
		if (!m_ctx->graphics_state.test(rsx::pipeline_state::vertex_program_ucode_dirty))
		{
			return;
		}

		m_ctx->graphics_state.clear(rsx::pipeline_state::vertex_program_ucode_dirty);

		// Reload transform constants unconditionally for now
		m_ctx->graphics_state |= rsx::pipeline_state::transform_constants_dirty;

		const u32 transform_program_start = REGS(m_ctx)->transform_program_start();
		m_ctx->program.vertex_program.data.reserve(512 * 4);
		m_ctx->program.vertex_program.jump_table.clear();

		m_ctx->program.vp_metadata = program_hash_util::vertex_program_utils::analyse_vertex_program
		(
			REGS(m_ctx)->transform_program.data(),  // Input raw block
			transform_program_start,                // Address of entry point
			m_ctx->program.vertex_program           // [out] Program object
		);

		m_ctx->program.vertex_program.texture_state.import(m_ctx->program.vp_texture_state, m_ctx->program.vp_metadata.referenced_textures_mask);

		// Verify current texture state is valid
		if (m_ctx->textures.vertex_textures_dirty & m_ctx->program.vp_metadata.referenced_textures_mask)
		{
			m_ctx->graphics_state |= rsx::pipeline_state::vertex_program_state_dirty;
		}
	}

	void graph_engine::analyse_current_rsx_pipeline()
	{
		m_ctx->program.cache_hint.invalidate(m_ctx->graphics_state.load());

		if (u32 export_ctrl = get_fragment_program_export_config();
			(m_ctx->program.fragment_program.ctrl & fs_export_config_mask) != export_ctrl)
		{
			// Update control bits for immediate consumers
			m_ctx->program.fragment_program.ctrl &= ~fs_export_config_mask;
			m_ctx->program.fragment_program.ctrl |= export_ctrl;

			// Signal backend to reload pipeline
			m_ctx->graphics_state.set(rsx::pipeline_state::fragment_program_state_dirty);
		}

		prefetch_vertex_program();
		prefetch_fragment_program();
	}

	void graph_engine::get_current_vertex_program(const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::vertex_textures_count>& sampler_descriptors)
	{
		if (m_ctx->graphics_state.test(rsx::pipeline_state::xform_instancing_state_dirty))
		{
			m_ctx->program.vertex_program.ctrl &= ~RSX_SHADER_CONTROL_INSTANCED_CONSTANTS;
			if (REGS(m_ctx)->current_draw_clause.is_trivial_instanced_draw)
			{
				m_ctx->program.vertex_program.ctrl |= RSX_SHADER_CONTROL_INSTANCED_CONSTANTS;
			}

			m_ctx->graphics_state.clear(rsx::pipeline_state::xform_instancing_state_dirty);

			// Emit invalidate here in case ucode is actually clean
			m_ctx->program.cache_hint.invalidate_vertex_program(m_ctx->program.vertex_program);
		}

		if (const bool emulate_depth_range = requires_depth_range_emulation();
			emulate_depth_range != !!(m_ctx->program.vertex_program.ctrl & RSX_SHADER_CONTROL_EMULATE_DEPTH_RANGE))
		{
			m_ctx->program.vertex_program.ctrl ^= RSX_SHADER_CONTROL_EMULATE_DEPTH_RANGE;
			m_ctx->program.cache_hint.invalidate_vertex_program(m_ctx->program.vertex_program);
		}

		if (!m_ctx->graphics_state.test(rsx::pipeline_state::vertex_program_dirty))
		{
			return;
		}

		ensure(!m_ctx->graphics_state.test(rsx::pipeline_state::vertex_program_ucode_dirty));
		m_ctx->program.vertex_program.output_mask = REGS(m_ctx)->vertex_attrib_output_mask();

		m_ctx->program.vertex_program.ctrl &= ~RSX_SHADER_CONTROL_FLAT_SHADING;
		if (REGS(m_ctx)->shade_mode() == rsx::shading_mode::flat &&
			RSX(m_ctx)->get_backend_config().supports_last_provoking_vertex)
		{
			m_ctx->program.vertex_program.ctrl |= RSX_SHADER_CONTROL_FLAT_SHADING;
		}

		for (u32 textures_ref = m_ctx->program.vp_metadata.referenced_textures_mask, i = 0; textures_ref; textures_ref >>= 1, ++i)
		{
			if (!(textures_ref & 1)) continue;

			const auto &tex = REGS(m_ctx)->vertex_textures[i];
			if (tex.enabled() && (m_ctx->program.vp_metadata.referenced_textures_mask & (1 << i)))
			{
				m_ctx->program.vp_texture_state.clear(i);
				m_ctx->program.vp_texture_state.set_dimension(sampler_descriptors[i]->image_type, i);

				if (RSX(m_ctx)->get_backend_config().supports_hw_msaa &&
					sampler_descriptors[i]->samples > 1)
				{
					m_ctx->program.vp_texture_state.multisampled_textures |= (1 << i);
				}
			}
		}

		m_ctx->program.vertex_program.texture_state.import(m_ctx->program.vp_texture_state, m_ctx->program.vp_metadata.referenced_textures_mask);

		m_ctx->program.cache_hint.invalidate_vertex_program(m_ctx->program.vertex_program);
	}

	void graph_engine::get_current_fragment_program(const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::fragment_textures_count>& sampler_descriptors)
	{
		if (!m_ctx->graphics_state.test(rsx::pipeline_state::fragment_program_dirty))
		{
			return;
		}

		ensure(!m_ctx->graphics_state.test(rsx::pipeline_state::fragment_program_ucode_dirty));

		m_ctx->graphics_state.clear(rsx::pipeline_state::fragment_program_dirty);

		m_ctx->program.fragment_program.ctrl &= fs_export_config_mask;
		m_ctx->program.fragment_program.ctrl |= REGS(m_ctx)->shader_control() & (CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS | CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT | RSX_SHADER_CONTROL_USES_KIL);
		m_ctx->program.fragment_program.texcoord_control_mask = REGS(m_ctx)->texcoord_control_mask();
		m_ctx->program.fragment_program.two_sided_lighting = REGS(m_ctx)->two_side_light_en();
		m_ctx->program.fragment_program.mrt_buffers_count = rsx::utility::get_mrt_buffers_count(REGS(m_ctx)->surface_color_target());

		if (REGS(m_ctx)->shade_mode() == rsx::shading_mode::flat &&
			RSX(m_ctx)->get_backend_config().supports_last_provoking_vertex)
		{
			m_ctx->program.fragment_program.ctrl |= RSX_SHADER_CONTROL_FLAT_SHADING;
		}

		if (REGS(m_ctx)->current_draw_clause.classify_mode() == primitive_class::polygon)
		{
			if (!RSX(m_ctx)->get_backend_config().supports_normalized_barycentrics)
			{
				m_ctx->program.fragment_program.ctrl |= RSX_SHADER_CONTROL_ATTRIBUTE_INTERPOLATION;
			}

			if (REGS(m_ctx)->polygon_stipple_enabled())
			{
				m_ctx->program.fragment_program.ctrl |= RSX_SHADER_CONTROL_POLYGON_STIPPLE;
			}
		}
		else if (REGS(m_ctx)->point_sprite_enabled() &&
			REGS(m_ctx)->current_draw_clause.primitive == primitive_type::points)
		{
			// Set high word of the control mask to store point sprite control
			m_ctx->program.fragment_program.texcoord_control_mask |= u32(REGS(m_ctx)->point_sprite_control_mask()) << 16;
		}

		if (REGS(m_ctx)->alpha_test_enabled())
		{
			m_ctx->program.fragment_program.ctrl |= RSX_SHADER_CONTROL_ALPHA_TEST;
		}

		if (REGS(m_ctx)->msaa_alpha_to_coverage_enabled())
		{
			const bool is_multiple_samples = REGS(m_ctx)->surface_antialias() != rsx::surface_antialiasing::center_1_sample;
			if (!RSX(m_ctx)->get_backend_config().supports_hw_a2c || (!is_multiple_samples && !RSX(m_ctx)->get_backend_config().supports_hw_a2c_1spp))
			{
				// Emulation required
				m_ctx->program.fragment_program.ctrl |= RSX_SHADER_CONTROL_ALPHA_TO_COVERAGE;
			}
		}

		// Check if framebuffer is actually an XRGB format and not a WZYX format
		switch (REGS(m_ctx)->surface_color())
		{
		case rsx::surface_color_format::w16z16y16x16:
		case rsx::surface_color_format::w32z32y32x32:
		case rsx::surface_color_format::x32:
			// These behave very differently from "normal" formats.
			break;
		default:
			// Integer framebuffer formats. These can support sRGB output as well as some special rules for output quantization.
			m_ctx->program.fragment_program.ctrl |= RSX_SHADER_CONTROL_8BIT_FRAMEBUFFER;
			if (!(m_ctx->program.fragment_program.ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS) && // Cannot output sRGB from 32-bit registers
				REGS(m_ctx)->framebuffer_srgb_enabled())
			{
				m_ctx->program.fragment_program.ctrl |= RSX_SHADER_CONTROL_SRGB_FRAMEBUFFER;
			}
			if (REGS(m_ctx)->surface_is_swizzle_remapped())
			{
				m_ctx->program.fragment_program.ctrl |= RSX_SHADER_CONTROL_ROP_OUTPUT_REMAP;
			}
			break;
		}

		const bool zeta_was_cyclic = m_ctx->graphics_state & rsx::zeta_address_is_cyclic;
		m_ctx->graphics_state.clear(rsx::zeta_address_is_cyclic);

		for (u32 textures_ref = m_ctx->program.fp_metadata.referenced_textures_mask, i = 0; textures_ref; textures_ref >>= 1, ++i)
		{
			if (!(textures_ref & 1)) continue;

			auto &tex = REGS(m_ctx)->fragment_textures[i];
			m_ctx->program.fp_texture_state.clear(i);

			if (!tex.enabled() || sampler_descriptors[i]->format_class == RSX_FORMAT_CLASS_UNDEFINED)
			{
				continue;
			}

			std::memcpy(
				m_ctx->program.fragment_program.texture_params[i].scale,
				sampler_descriptors[i]->texcoord_xform.scale,
				sizeof(sampler_descriptors[i]->texcoord_xform.scale) * 2); // Copy scale and bias together

			m_ctx->program.fragment_program.texture_params[i].remap = tex.remap();

			m_ctx->graphics_state |= rsx::pipeline_state::fragment_texture_state_dirty;

			u32 texture_control = 0;
			m_ctx->program.fp_texture_state.set_dimension(sampler_descriptors[i]->image_type, i);

			if (sampler_descriptors[i]->texcoord_xform.clamp)
			{
				std::memcpy(
					m_ctx->program.fragment_program.texture_params[i].clamp_min,
					sampler_descriptors[i]->texcoord_xform.clamp_min,
					sizeof(sampler_descriptors[i]->texcoord_xform.clamp_min) * 2); // Copy clamp_min and clamp_max together

				texture_control |= (1 << rsx::texture_control_bits::CLAMP_TEXCOORDS_BIT);
			}

			if (tex.alpha_kill_enabled())
			{
				//alphakill can be ignored unless a valid comparison function is set
				texture_control |= (1 << texture_control_bits::ALPHAKILL);
				m_ctx->program.fragment_program.ctrl |= RSX_SHADER_CONTROL_TEXTURE_ALPHA_KILL;
			}

			//const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
			const u32 raw_format = tex.format();
			const u32 format = raw_format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);

			if (raw_format & CELL_GCM_TEXTURE_UN)
			{
				if (tex.min_filter() == rsx::texture_minify_filter::nearest ||
					tex.mag_filter() == rsx::texture_magnify_filter::nearest)
				{
					// Subpixel offset so that (X + bias) * scale will round correctly.
					// This is done to work around fdiv precision issues in some GPUs (NVIDIA)
					// We apply the simplification where (x + bias) * z = xz + zbias here.
					constexpr auto subpixel_bias = 0.01f;
					m_ctx->program.fragment_program.texture_params[i].bias[0] += (subpixel_bias * m_ctx->program.fragment_program.texture_params[i].scale[0]);
					m_ctx->program.fragment_program.texture_params[i].bias[1] += (subpixel_bias * m_ctx->program.fragment_program.texture_params[i].scale[1]);
					m_ctx->program.fragment_program.texture_params[i].bias[2] += (subpixel_bias * m_ctx->program.fragment_program.texture_params[i].scale[2]);
				}
			}

			if (RSX(m_ctx)->get_backend_config().supports_hw_msaa && sampler_descriptors[i]->samples > 1)
			{
				m_ctx->program.fp_texture_state.multisampled_textures |= (1 << i);
				texture_control |= (static_cast<u32>(tex.zfunc()) << texture_control_bits::DEPTH_COMPARE_OP);
				texture_control |= (static_cast<u32>(tex.mag_filter() != rsx::texture_magnify_filter::nearest) << texture_control_bits::FILTERED_MAG);
				texture_control |= (static_cast<u32>(tex.min_filter() != rsx::texture_minify_filter::nearest) << texture_control_bits::FILTERED_MIN);
				texture_control |= (((tex.format() & CELL_GCM_TEXTURE_UN) >> 6) << texture_control_bits::UNNORMALIZED_COORDS);

				if (rsx::is_texcoord_wrapping_mode(tex.wrap_s()))
				{
					texture_control |= (1 << texture_control_bits::WRAP_S);
				}

				if (rsx::is_texcoord_wrapping_mode(tex.wrap_t()))
				{
					texture_control |= (1 << texture_control_bits::WRAP_T);
				}

				if (rsx::is_texcoord_wrapping_mode(tex.wrap_r()))
				{
					texture_control |= (1 << texture_control_bits::WRAP_R);
				}
			}

			if (sampler_descriptors[i]->format_class != RSX_FORMAT_CLASS_COLOR)
			{
				switch (sampler_descriptors[i]->format_class)
				{
				case RSX_FORMAT_CLASS_DEPTH16_FLOAT:
				case RSX_FORMAT_CLASS_DEPTH24_FLOAT_X8_PACK32:
					texture_control |= (1 << texture_control_bits::DEPTH_FLOAT);
					break;
				default:
					break;
				}

				switch (format)
				{
				case CELL_GCM_TEXTURE_A8R8G8B8:
				case CELL_GCM_TEXTURE_D8R8G8B8:
				{
					// Emulate bitcast in shader
					m_ctx->program.fp_texture_state.redirected_textures |= (1 << i);
					const auto float_en = (sampler_descriptors[i]->format_class == RSX_FORMAT_CLASS_DEPTH24_FLOAT_X8_PACK32)? 1 : 0;
					texture_control |= (float_en << texture_control_bits::DEPTH_FLOAT);
					break;
				}
				case CELL_GCM_TEXTURE_X16:
				{
					// A simple way to quickly read DEPTH16 data without shadow comparison
					break;
				}
				case CELL_GCM_TEXTURE_DEPTH16:
				case CELL_GCM_TEXTURE_DEPTH24_D8:
				case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
				case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
				{
					// Natively supported Z formats with shadow comparison feature
					const auto compare_mode = tex.zfunc();
					if (!tex.alpha_kill_enabled() &&
						compare_mode < rsx::comparison_function::always &&
						compare_mode > rsx::comparison_function::never)
					{
						m_ctx->program.fp_texture_state.shadow_textures |= (1 << i);
					}
					break;
				}
				default:
					rsx_log.error("Depth texture bound to pipeline with unexpected format 0x%X", format);
				}

				if (sampler_descriptors[i]->is_cyclic_reference &&
					m_ctx->fb.layout.zeta_address != 0 &&
					!g_cfg.video.strict_rendering_mode &&
					g_cfg.video.shader_precision != gpu_preset_level::low)
				{
					m_ctx->graphics_state |= rsx::zeta_address_is_cyclic;

					if (!(m_ctx->program.fragment_program.ctrl & (CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT | RSX_SHADER_CONTROL_META_USES_DISCARD | RSX_SHADER_CONTROL_EMULATE_DEPTH_COMPARE)) &&
						m_ctx->fb.layout.zeta_write_enabled)
					{
						m_ctx->program.fragment_program.ctrl |= RSX_SHADER_CONTROL_DISABLE_EARLY_Z;
					}
				}
			}
			else if (!RSX(m_ctx)->get_backend_config().supports_hw_renormalization /* &&
				tex.min_filter() == rsx::texture_minify_filter::nearest &&
				tex.mag_filter() == rsx::texture_magnify_filter::nearest*/)
			{
				// FIXME: This check should only apply to point-sampled textures. However, it severely regresses some games (id tech 5).
				// This is because even when filtering is active, the error from the PS3 texture expansion still applies.
				// A proper fix is to expand these formats into BGRA8 when high texture precision is required. That requires different GUI settings and inflation shaders, so it will be handled separately.

				switch (format)
				{
				case CELL_GCM_TEXTURE_A1R5G5B5:
				case CELL_GCM_TEXTURE_A4R4G4B4:
				case CELL_GCM_TEXTURE_D1R5G5B5:
				case CELL_GCM_TEXTURE_R5G5B5A1:
				case CELL_GCM_TEXTURE_R5G6B5:
				case CELL_GCM_TEXTURE_R6G5B5:
					texture_control |= (1 << texture_control_bits::RENORMALIZE);
					m_ctx->program.fragment_program.ctrl |= RSX_SHADER_CONTROL_TEXTURE_FORMAT_CONVERT;
					break;
				default:
					break;
				}
			}

			if (const auto& format_ex = sampler_descriptors[i]->format_ex; format_ex.features != 0)
			{
				texture_control |= format_ex.texel_remap_control;
				texture_control |= format_ex.features << texture_control_bits::FORMAT_FEATURES_OFFSET;

				if (format_ex.texel_remap_control)
				{
					m_ctx->program.fragment_program.ctrl |= RSX_SHADER_CONTROL_TEXTURE_FORMAT_CONVERT;
				}

				if (m_ctx->program.fp_metadata.bx2_texture_reads_mask)
				{
					m_ctx->program.fragment_program.ctrl |= RSX_SHADER_CONTROL_TEXTURE_FORMAT_CONVERT;

					const u32 remap_hi = tex.decoded_remap().shuffle_mask_bits(0xFu);
					m_ctx->program.fragment_program.texture_params[i].remap &= ~(0xFu << 16u);
					m_ctx->program.fragment_program.texture_params[i].remap |= (remap_hi << 16u);
				}
			}

			m_ctx->program.fragment_program.texture_params[i].control = texture_control;
		}

		// Update texture configuration
		m_ctx->program.fragment_program.texture_state.import(m_ctx->program.fp_texture_state, m_ctx->program.fp_metadata.referenced_textures_mask);

		//Sanity checks
		if (m_ctx->program.fragment_program.ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
		{
			//Check that the depth stage is not disabled
			if (!REGS(m_ctx)->depth_test_enabled())
			{
				rsx_log.trace("FS exports depth component but depth test is disabled (INVALID_OPERATION)");
			}
		}

		m_ctx->program.cache_hint.invalidate_fragment_program(m_ctx->program.fragment_program);

		if (zeta_was_cyclic && zeta_was_cyclic != m_ctx->graphics_state.test(rsx::zeta_address_is_cyclic))
		{
			// Forced "fall-out" barrier. This is a special case for Z buffers because they can be cyclic without writes.
			// That condition can cause early-Z in a later call to introduce data hazard in previous cyclic draws.
			m_ctx->graphics_state |= rsx::zeta_address_cyclic_barrier;
		}
	}

	bool graph_engine::invalidate_fragment_program(u32 dst_dma, u32 dst_offset, u32 size)
	{
		if (!m_ctx->program.fragment_program.total_length)
		{
			// No shader loaded
			return false;
		}

		const auto [shader_offset, shader_dma] = REGS(m_ctx)->shader_program_address();
		if ((dst_dma & CELL_GCM_LOCATION_MAIN) != shader_dma)
		{
			// Shader not loaded in XDR memory
			return false;
		}

		const auto current_fragment_shader_range = address_range32::start_length(shader_offset, m_ctx->program.fragment_program.total_length);
		if (!current_fragment_shader_range.overlaps(address_range32::start_length(dst_offset, size)))
		{
			// No range overlap
			return false;
		}

		// Data overlaps. Force ucode reload.
		m_ctx->graphics_state |= rsx::pipeline_state::fragment_program_ucode_dirty;
		return true;
	}
}
