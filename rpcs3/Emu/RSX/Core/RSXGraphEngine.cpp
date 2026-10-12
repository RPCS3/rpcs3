#include "stdafx.h"
#include "RSXGraphEngine.h"

#include "Emu/RSX/Common/surface_store.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/rsx_methods.h"
#include "Emu/system_config.h"

#include "Emu/RSX/NV47/HW/context_accessors.define.h"

namespace rsx
{
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

	bool graph_engine::get_scissor(areau& region, bool clip_viewport, const surface_scaling_config_t& resolution_scaling_config)
	{
		if (!m_ctx->graphics_state.test(rsx::pipeline_state::scissor_config_state_dirty))
		{
			if (clip_viewport == m_ctx->graphics_state.test(rsx::pipeline_state::scissor_setup_clipped))
			{
				// Nothing to do
				return false;
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
			return false;
		}

		if (m_ctx->graphics_state & rsx::pipeline_state::scissor_setup_invalid)
		{
			m_ctx->graphics_state.clear(rsx::pipeline_state::scissor_setup_invalid);
			m_ctx->graphics_state.set(rsx::rtt_config_valid);
		}

		std::tie(region.x1, region.y1) = rsx::apply_resolution_scale<false>(resolution_scaling_config, x1, y1, m_ctx->fb.layout.width, m_ctx->fb.layout.height);
		std::tie(region.x2, region.y2) = rsx::apply_resolution_scale<true>(resolution_scaling_config, x2, y2, m_ctx->fb.layout.width, m_ctx->fb.layout.height);

		return true;
	}
}
