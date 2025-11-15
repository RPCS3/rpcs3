#include "stdafx.h"
#include "GLGSRender.h"

#include "upscalers/bilinear_pass.hpp"
#include "upscalers/fsr_pass.h"
#include "upscalers/nearest_pass.hpp"

#include "Emu/Cell/Modules/cellVideoOut.h"
#include "Emu/RSX/Overlays/overlay_manager.h"
#include "Emu/RSX/Overlays/overlay_debug_overlay.h"

#include "util/video_provider.h"

LOG_CHANNEL(screenshot_log, "SCREENSHOT");

extern atomic_t<bool> g_user_asked_for_screenshot;
extern atomic_t<recording_mode> g_recording_mode;

namespace gl
{
	namespace debug
	{
		std::unique_ptr<texture> g_vis_texture;

		void set_vis_texture(texture* visual)
		{
			const auto target = static_cast<GLenum>(visual->get_target());
			const auto ifmt = static_cast<GLenum>(visual->get_internal_format());
			g_vis_texture.reset(new texture(target, visual->width(), visual->height(), 1, 1, 1, ifmt, visual->format_class()));
			glCopyImageSubData(visual->id(), target, 0, 0, 0, 0, g_vis_texture->id(), target, 0, 0, 0, 0, visual->width(), visual->height(), 1);
		}
	}

	GLenum RSX_display_format_to_gl_format(u8 format)
	{
		switch (format)
		{
		default:
			rsx_log.error("Unhandled video output format 0x%x", static_cast<s32>(format));
			[[fallthrough]];
		case CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8:
			return GL_BGRA8;
		case CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8B8G8R8:
			return GL_RGBA8;
		case CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_R16G16B16X16_FLOAT:
			return GL_RGBA16F;
		}
	}
}

gl::texture* GLGSRender::get_present_source(gl::present_surface_info* info, const rsx::avconf& avconfig)
{
	gl::texture* image = nullptr;

	// @FIXME: This implementation needs to merge into the texture cache's upload_texture routine.
	// See notes on the vulkan implementation on what needs to happen before that is viable.

	// Check the surface store first
	gl::command_context cmd = { gl_state };
	const auto format_bpp = rsx::get_format_block_size_in_bytes(info->format);
	const auto overlap_info = m_rtts.get_merged_texture_memory_region(cmd,
		info->address, info->width, info->height, info->pitch, format_bpp, rsx::surface_access::transfer_read);

	if (!overlap_info.empty())
	{
		const auto& section = overlap_info.back();
		auto surface = gl::as_rtt(section.surface);
		bool viable = false;

		if (section.base_address >= info->address)
		{
			const auto surface_width = surface->get_surface_width<rsx::surface_metrics::samples>();
			const auto surface_height = surface->get_surface_height<rsx::surface_metrics::samples>();

			if (section.base_address == info->address)
			{
				// Check for fit or crop
				viable = (surface_width >= info->width && surface_height >= info->height);
			}
			else
			{
				// Check for borders and letterboxing
				const u32 inset_offset = section.base_address - info->address;
				const u32 inset_y = inset_offset / info->pitch;
				const u32 inset_x = (inset_offset % info->pitch) / format_bpp;

				const u32 full_width = surface_width + inset_x + inset_x;
				const u32 full_height = surface_height + inset_y + inset_y;

				viable = (full_width == info->width && full_height == info->height);
			}

			if (viable)
			{
				image = section.surface->get_surface(rsx::surface_access::transfer_read);

				std::tie(info->width, info->height) = rsx::apply_resolution_scale<true>(
					std::min(surface_width, info->width),
					std::min(surface_height, info->height));
			}
		}
	}
	else if (auto surface = m_gl_texture_cache.find_texture_from_dimensions<true>(info->address, info->format);
			 surface && surface->get_width() >= info->width && surface->get_height() >= info->height)
	{
		// Hack - this should be the first location to check for output
		// The render might have been done offscreen or in software and a blit used to display
		if (const auto tex = surface->get_raw_texture(); tex) image = tex;
	}

	const GLenum expected_format = gl::RSX_display_format_to_gl_format(avconfig.format);
	std::unique_ptr<gl::texture>& flip_image = m_flip_tex_color[info->eye];

	auto initialize_scratch_image = [&]()
	{
		if (!flip_image || flip_image->size2D() != sizeu{ info->width, info->height })
		{
			flip_image = std::make_unique<gl::texture>(GL_TEXTURE_2D, info->width, info->height, 1, 1, 1, expected_format, RSX_FORMAT_CLASS_COLOR);
		}
	};

	if (!image)
	{
		rsx_log.warning("Flip texture was not found in cache. Uploading surface from CPU");

		gl::pixel_unpack_settings unpack_settings;
		unpack_settings.alignment(1).row_length(info->pitch / 4);

		initialize_scratch_image();

		gl::command_context cmd{ gl_state };
		const auto range = utils::address_range32::start_length(info->address, info->pitch * info->height);
		m_gl_texture_cache.invalidate_range(cmd, range, rsx::invalidation_cause::read);

		flip_image->copy_from(vm::base(info->address), static_cast<gl::texture::format>(expected_format), gl::texture::type::uint_8_8_8_8, unpack_settings);
		image = flip_image.get();
	}
	else if (image->get_internal_format() != static_cast<gl::texture::internal_format>(expected_format))
	{
		initialize_scratch_image();

		// Copy
		if (gl::formats_are_bitcast_compatible(flip_image.get(), image))
		{
			const position3u offset{};
			gl::g_hw_blitter->copy_image(cmd, image, flip_image.get(), 0, 0, offset, offset, { info->width, info->height, 1 });
		}
		else
		{
			const coord3u region = { {/* offsets */}, { info->width, info->height, 1 } };
			gl::copy_typeless(cmd, flip_image.get(), image, region, region);
		}

		image = flip_image.get();
	}

	return image;
}

void GLGSRender::flip(const rsx::display_flip_info_t& info)
{
	if (info.skip_frame)
	{
		m_frame->flip(m_context, true);
		rsx::thread::flip(info);
		return;
	}

	gl::command_context cmd{ gl_state };

	u32 buffer_width = display_buffers[info.buffer].width;
	u32 buffer_height = display_buffers[info.buffer].height;
	u32 buffer_pitch = display_buffers[info.buffer].pitch;

	u32 av_format;
	const auto& avconfig = g_fxo->get<rsx::avconf>();

	if (!buffer_width)
	{
		buffer_width = avconfig.resolution_x;
		buffer_height = avconfig.resolution_y;
	}

	if (avconfig.state)
	{
		av_format = avconfig.get_compatible_gcm_format();
		if (!buffer_pitch)
			buffer_pitch = buffer_width * avconfig.get_bpp();

		const size2u video_frame_size = avconfig.video_frame_size();
		buffer_width = std::min(buffer_width, video_frame_size.width);
		buffer_height = std::min(buffer_height, video_frame_size.height);
	}
	else
	{
		av_format = CELL_GCM_TEXTURE_A8R8G8B8;
		if (!buffer_pitch)
			buffer_pitch = buffer_width * 4;
	}

	// Disable scissor test (affects blit, clear, etc)
	gl_state.disable(GL_SCISSOR_TEST);

	// Enable drawing to window backbuffer
	gl::screen.bind();

	gl::texture* image_to_flip = nullptr;
	gl::texture* image_to_flip2 = nullptr;

	if (info.buffer < display_buffers_count && buffer_width && buffer_height)
	{
		// Find the source image
		gl::present_surface_info present_info
		{
			.address = rsx::get_address(display_buffers[info.buffer].offset, CELL_GCM_LOCATION_LOCAL),
			.format = av_format,
			.width = buffer_width,
			.height = buffer_height,
			.pitch = buffer_pitch,
			.eye = 0
		};

		image_to_flip = get_present_source(&present_info, avconfig);

		if (avconfig.stereo_enabled) [[unlikely]]
		{
			const auto [unused, min_expected_height] = rsx::apply_resolution_scale<true>(RSX_SURFACE_DIMENSION_IGNORED, buffer_height + 30);
			if (image_to_flip->height() < min_expected_height)
			{
				// Get image for second eye
				const u32 image_offset = (buffer_height + 30) * buffer_pitch + display_buffers[info.buffer].offset;
				present_info.width = buffer_width;
				present_info.height = buffer_height;
				present_info.address = rsx::get_address(image_offset, CELL_GCM_LOCATION_LOCAL);
				present_info.eye = 1;

				image_to_flip2 = get_present_source(&present_info, avconfig);
			}
			else
			{
				// Account for possible insets
				const auto [unused2, scaled_buffer_height] = rsx::apply_resolution_scale<true>(RSX_SURFACE_DIMENSION_IGNORED, buffer_height);
				buffer_height = std::min<u32>(image_to_flip->height() - min_expected_height, scaled_buffer_height);
			}
		}

		buffer_width = present_info.width;
		buffer_height = present_info.height;
	}

	if (info.emu_flip)
	{
		evaluate_cpu_usage_reduction_limits();
	}

	// Get window state
	const int width = m_frame->client_width();
	const int height = m_frame->client_height();

	// Calculate blit coordinates
	areai aspect_ratio;
	if (!g_cfg.video.stretch_to_display_area)
	{
		const sizeu csize(width, height);
		const auto converted = avconfig.aspect_convert_region(size2u{ buffer_width, buffer_height }, csize);
		aspect_ratio = static_cast<areai>(converted);
	}
	else
	{
		aspect_ratio = { 0, 0, width, height };
	}

	if (!image_to_flip || aspect_ratio.x1 || aspect_ratio.y1)
	{
		// Clear the window background to opaque black
		gl_state.clear_color(0, 0, 0, 255);
		gl::screen.clear(gl::buffers::color);
	}

	if (m_overlay_manager && m_overlay_manager->has_dirty())
	{
		m_overlay_manager->lock_shared();

		std::vector<u32> uids_to_dispose;
		uids_to_dispose.reserve(m_overlay_manager->get_dirty().size());

		for (const auto& view : m_overlay_manager->get_dirty())
		{
			m_ui_renderer.remove_temp_resources(view->uid);
			uids_to_dispose.push_back(view->uid);
		}

		m_overlay_manager->unlock_shared();
		m_overlay_manager->dispose(uids_to_dispose);
	}

	const auto render_overlays = [this, &cmd](gl::texture* dst, const areau& aspect_ratio, bool flip_vertically = false)
	{
		if (m_overlay_manager && m_overlay_manager->has_visible())
		{
			GLuint target = 0;

			if (dst)
			{
				m_sshot_fbo.bind();
				m_sshot_fbo.color = dst->id();
				target = dst->id();
			}
			else
			{
				gl::screen.bind();
			}

			// Lock to avoid modification during run-update chain
			std::lock_guard lock(*m_overlay_manager);

			for (const auto& view : m_overlay_manager->get_views())
			{
				m_ui_renderer.run(cmd, aspect_ratio, target, *view.get(), flip_vertically);
			}
		}
	};

	if (image_to_flip)
	{
		const bool user_asked_for_screenshot = g_user_asked_for_screenshot.exchange(false);

		if (user_asked_for_screenshot || (g_recording_mode != recording_mode::stopped && m_frame->can_consume_frame()))
		{
			static const gl::pixel_pack_settings pack_settings{};

			gl::texture* tex = image_to_flip;

			if (g_cfg.video.record_with_overlays)
			{
				m_sshot_fbo.create();

				if (!m_sshot_tex ||
					m_sshot_tex->get_target() != image_to_flip->get_target() ||
					m_sshot_tex->width() != image_to_flip->width() ||
					m_sshot_tex->height() != image_to_flip->height() ||
					m_sshot_tex->depth() != image_to_flip->depth() ||
					m_sshot_tex->levels() != image_to_flip->levels() ||
					m_sshot_tex->samples() != image_to_flip->samples() ||
					m_sshot_tex->get_internal_format() != image_to_flip->get_internal_format() ||
					m_sshot_tex->format_class() != image_to_flip->format_class())
				{
					m_sshot_tex = std::make_unique<gl::texture>(
						GLenum(image_to_flip->get_target()),
						image_to_flip->width(),
						image_to_flip->height(),
						image_to_flip->depth(),
						image_to_flip->levels(),
						image_to_flip->samples(),
						GLenum(image_to_flip->get_internal_format()),
						image_to_flip->format_class());
				}

				tex = m_sshot_tex.get();

				static const position3u offset{};
				gl::g_hw_blitter->copy_image(cmd, image_to_flip, tex, 0, 0, offset, offset, { tex->width(), tex->height(), 1 });

				render_overlays(tex, areau(0, 0, image_to_flip->width(), image_to_flip->height()), true);
				m_sshot_fbo.remove();
			}
			
			std::vector<u8> sshot_frame(buffer_height * buffer_width * 4);
			glGetError();

			tex->copy_to(sshot_frame.data(), gl::texture::format::rgba, gl::texture::type::ubyte, pack_settings);

			m_sshot_tex.reset();

			if (GLenum err = glGetError(); err != GL_NO_ERROR)
			{
				screenshot_log.error("Failed to capture image: 0x%x", err);
			}
			else if (user_asked_for_screenshot)
			{
				m_frame->take_screenshot(std::move(sshot_frame), buffer_width, buffer_height, false);
			}
			else
			{
				m_frame->present_frame(std::move(sshot_frame), buffer_width * 4, buffer_width, buffer_height, false);
			}
		}

		const areai screen_area = coordi({}, { static_cast<int>(buffer_width), static_cast<int>(buffer_height) });
		const bool use_full_rgb_range_output = g_cfg.video.full_rgb_range_output.get();
		const bool backbuffer_has_alpha = m_frame->has_alpha();

		if (!m_upscaler || m_output_scaling != g_cfg.video.output_scaling)
		{
			m_output_scaling = g_cfg.video.output_scaling;

			switch (m_output_scaling)
			{
			case output_scaling_mode::nearest:
				m_upscaler = std::make_unique<gl::nearest_upscale_pass>();
				break;
			case output_scaling_mode::fsr:
				m_upscaler = std::make_unique<gl::fsr_upscale_pass>();
				break;
			case output_scaling_mode::bilinear:
			default:
				m_upscaler = std::make_unique<gl::bilinear_upscale_pass>();
				break;
			}
		}

		if (!backbuffer_has_alpha && use_full_rgb_range_output && rsx::fcmp(avconfig.gamma, 1.f) && !avconfig.stereo_enabled)
		{
			// Blit source image to the screen
			m_upscaler->scale_output(cmd, image_to_flip, screen_area, aspect_ratio.flipped_vertical(), UPSCALE_AND_COMMIT | UPSCALE_DEFAULT_VIEW);
		}
		else
		{
			const f32 gamma = avconfig.gamma;
			const bool limited_range = !use_full_rgb_range_output;
			const auto filter = m_output_scaling == output_scaling_mode::nearest ? gl::filter::nearest : gl::filter::linear;
			rsx::simple_array<gl::texture*> images{ image_to_flip, image_to_flip2 };

			if (m_output_scaling == output_scaling_mode::fsr && !avconfig.stereo_enabled) // 3D will be implemented later
			{
				for (unsigned i = 0; i < 2 && images[i]; ++i)
				{
					const rsx::flags32_t mode = (i == 0) ? UPSCALE_LEFT_VIEW : UPSCALE_RIGHT_VIEW;
					images[i] = m_upscaler->scale_output(cmd, image_to_flip, screen_area, aspect_ratio.flipped_vertical(), mode);
				}
			}

			gl::screen.bind();
			m_video_output_pass.run(cmd, areau(aspect_ratio), images.map(FN(x ? x->id() : GL_NONE)), gamma, limited_range, avconfig.stereo_enabled, g_cfg.video.stereo_render_mode, filter);
		}
	}

	render_overlays(nullptr, areau(aspect_ratio));

	if (g_cfg.video.debug_overlay)
	{
		const auto num_dirty_textures = m_gl_texture_cache.get_unreleased_textures_count();
		const auto texture_memory_size = m_gl_texture_cache.get_texture_memory_in_use() / (1024 * 1024);
		const auto num_flushes = m_gl_texture_cache.get_num_flush_requests();
		const auto num_mispredict = m_gl_texture_cache.get_num_cache_mispredictions();
		const auto num_speculate = m_gl_texture_cache.get_num_cache_speculative_writes();
		const auto num_misses = m_gl_texture_cache.get_num_cache_misses();
		const auto num_unavoidable = m_gl_texture_cache.get_num_unavoidable_hard_faults();
		const auto cache_miss_ratio = static_cast<u32>(ceil(m_gl_texture_cache.get_cache_miss_ratio() * 100));
		const auto num_texture_upload = m_gl_texture_cache.get_texture_upload_calls_this_frame();
		const auto num_texture_upload_miss = m_gl_texture_cache.get_texture_upload_misses_this_frame();
		const auto texture_upload_miss_ratio = m_gl_texture_cache.get_texture_upload_miss_percentage();
		const auto texture_copies_ellided = m_gl_texture_cache.get_texture_copies_ellided_this_frame();
		const auto vertex_cache_hit_count = (info.stats.vertex_cache_request_count - info.stats.vertex_cache_miss_count);
		const auto vertex_cache_hit_ratio = info.stats.vertex_cache_request_count
			? (vertex_cache_hit_count * 100) / info.stats.vertex_cache_request_count
			: 0;
		const auto program_cache_lookups = info.stats.program_cache_lookups_total;
		const auto program_cache_ellided = info.stats.program_cache_lookups_ellided;
		const auto program_cache_ellision_rate = program_cache_lookups
			? (program_cache_ellided * 100) / program_cache_lookups
			: 0;

		rsx::overlays::set_debug_overlay_text(fmt::format(
			"Internal Resolution:     %s\n"
			"RSX Load:                %3d%%\n"
			"draw calls: %16d\n"
			"draw call setup: %11dus\n"
			"vertex upload time: %8dus\n"
			"textures upload time: %6dus\n"
			"draw call execution: %7dus\n"
			"Unreleased textures: %7d\n"
			"Texture memory: %12dM\n"
			"Flush requests: %12d  = %2d (%3d%%) hard faults, %2d unavoidable, %2d misprediction(s), %2d speculation(s)\n"
			"Texture uploads: %11u (%u from CPU - %02u%%, %u copies avoided)\n"
			"Vertex cache hits: %9u/%u (%u%%)\n"
			"Program cache lookup ellision: %u/%u (%u%%)",
			info.stats.framebuffer_stats.to_string(!backend_config.supports_hw_msaa),
			get_load(), info.stats.draw_calls, info.stats.setup_time, info.stats.vertex_upload_time,
			info.stats.textures_upload_time, info.stats.draw_exec_time, num_dirty_textures, texture_memory_size,
			num_flushes, num_misses, cache_miss_ratio, num_unavoidable, num_mispredict, num_speculate,
			num_texture_upload, num_texture_upload_miss, texture_upload_miss_ratio, texture_copies_ellided,
			vertex_cache_hit_count, info.stats.vertex_cache_request_count, vertex_cache_hit_ratio,
			program_cache_ellided, program_cache_lookups, program_cache_ellision_rate)
		);
	}

	if (gl::debug::g_vis_texture)
	{
		// Optionally renders a single debug texture to framebuffer.
		// Only programmatic access provided at the moment.
		// TODO: Migrate to use overlay system. (kd-11)
		gl::fbo m_vis_buffer;
		m_vis_buffer.create();
		m_vis_buffer.bind();
		m_vis_buffer.color = gl::debug::g_vis_texture->id();
		m_vis_buffer.read_buffer(m_vis_buffer.color);
		m_vis_buffer.draw_buffer(m_vis_buffer.color);

		const u32 vis_width = 320;
		const u32 vis_height = 240;
		areai display_view = areai(aspect_ratio).flipped_vertical();
		display_view.x1 = display_view.x2 - vis_width;
		display_view.y1 = vis_height;

		// Blit
		const auto src_region = areau{ 0u, 0u, gl::debug::g_vis_texture->width(), gl::debug::g_vis_texture->height() };
		m_vis_buffer.blit(gl::screen, static_cast<areai>(src_region), display_view, gl::buffers::color, gl::filter::linear);
		m_vis_buffer.remove();
	}

	m_frame->flip(m_context);
	rsx::thread::flip(info);

	// Cleanup
	m_gl_texture_cache.on_frame_end();
	m_vertex_cache->purge();

	auto removed_textures = m_rtts.trim(cmd);
	m_framebuffer_cache.remove_if([&](auto& fbo)
	{
		if (fbo.unused_check_count() >= 2) return true; // Remove if stale
		if (fbo.references_any(removed_textures)) return true; // Remove if any of the attachments is invalid

		return false;
	});

	if (m_draw_fbo && !m_graphics_state.test(rsx::rtt_config_dirty))
	{
		// Always restore the active framebuffer
		m_draw_fbo->bind();
		set_viewport();
		set_scissor(m_graphics_state & rsx::pipeline_state::scissor_setup_clipped);
	}
}
