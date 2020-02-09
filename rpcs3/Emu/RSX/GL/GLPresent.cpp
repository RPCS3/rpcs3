#include "stdafx.h"
#include "GLGSRender.h"

LOG_CHANNEL(screenshot);

GLuint GLGSRender::get_present_source(gl::present_surface_info* info, const rsx::avconf* avconfig)
{
	GLuint image = GL_NONE;

	// Check the surface store first
	gl::command_context cmd = { gl_state };
	const auto format_bpp = get_format_block_size_in_bytes(info->format);
	const auto overlap_info = m_rtts.get_merged_texture_memory_region(cmd,
		info->address, info->width, info->height, info->pitch, format_bpp, rsx::surface_access::read);

	if (!overlap_info.empty())
	{
		const auto& section = overlap_info.back();
		auto surface = gl::as_rtt(section.surface);
		bool viable = false;

		if (section.base_address >= info->address)
		{
			const auto surface_width = surface->get_surface_width(rsx::surface_metrics::samples);
			const auto surface_height = surface->get_surface_height(rsx::surface_metrics::samples);

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
				surface->read_barrier(cmd);
				image = section.surface->get_surface(rsx::surface_access::read)->id();

				info->width = rsx::apply_resolution_scale(std::min(surface_width, static_cast<u16>(info->width)), true);
				info->height = rsx::apply_resolution_scale(std::min(surface_height, static_cast<u16>(info->height)), true);
			}
		}
	}
	else if (auto surface = m_gl_texture_cache.find_texture_from_dimensions<true>(info->address, info->format, info->width, info->height))
	{
		// Hack - this should be the first location to check for output
		// The render might have been done offscreen or in software and a blit used to display
		if (const auto tex = surface->get_raw_texture(); tex) image = tex->id();
	}

	if (!image)
	{
		rsx_log.warning("Flip texture was not found in cache. Uploading surface from CPU");

		gl::pixel_unpack_settings unpack_settings;
		unpack_settings.alignment(1).row_length(info->pitch / 4);

		if (!m_flip_tex_color || m_flip_tex_color->size2D() != sizei{ static_cast<int>(info->width), static_cast<int>(info->height) })
		{
			m_flip_tex_color = std::make_unique<gl::texture>(GL_TEXTURE_2D, info->width, info->height, 1, 1, GL_RGBA8);
		}

		gl::command_context cmd{ gl_state };
		const auto range = utils::address_range::start_length(info->address, info->pitch * info->height);
		m_gl_texture_cache.invalidate_range(cmd, range, rsx::invalidation_cause::read);

		m_flip_tex_color->copy_from(vm::base(info->address), gl::texture::format::bgra, gl::texture::type::uint_8_8_8_8, unpack_settings);
		image = m_flip_tex_color->id();
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

	u32 buffer_width = display_buffers[info.buffer].width;
	u32 buffer_height = display_buffers[info.buffer].height;
	u32 buffer_pitch = display_buffers[info.buffer].pitch;

	u32 av_format;
	const auto avconfig = g_fxo->get<rsx::avconf>();

	if (avconfig->state)
	{
		av_format = avconfig->get_compatible_gcm_format();
		if (!buffer_pitch)
			buffer_pitch = buffer_width * avconfig->get_bpp();

		buffer_width = std::min(buffer_width, avconfig->resolution_x);
		buffer_height = std::min(buffer_height, avconfig->resolution_y);
	}
	else
	{
		av_format = CELL_GCM_TEXTURE_A8R8G8B8;
		if (!buffer_pitch)
			buffer_pitch = buffer_width * 4;
	}

	// Disable scissor test (affects blit, clear, etc)
	gl_state.enable(GL_FALSE, GL_SCISSOR_TEST);

	// Enable drawing to window backbuffer
	gl::screen.bind();

	GLuint image_to_flip = GL_NONE;

	if (info.buffer < display_buffers_count && buffer_width && buffer_height)
	{
		// Find the source image
		gl::present_surface_info present_info;
		present_info.width = buffer_width;
		present_info.height = buffer_height;
		present_info.pitch = buffer_pitch;
		present_info.format = av_format;
		present_info.address = rsx::get_address(display_buffers[info.buffer].offset, CELL_GCM_LOCATION_LOCAL, HERE);

		image_to_flip = get_present_source(&present_info, avconfig);
		buffer_width = present_info.width;
		buffer_height = present_info.height;
	}

	// Calculate blit coordinates
	coordi aspect_ratio;
	sizei csize(m_frame->client_width(), m_frame->client_height());
	sizei new_size = csize;

	if (!g_cfg.video.stretch_to_display_area)
	{
		const double aq = 1. * buffer_width / buffer_height;
		const double rq = 1. * new_size.width / new_size.height;
		const double q = aq / rq;

		if (q > 1.0)
		{
			new_size.height = static_cast<int>(new_size.height / q);
			aspect_ratio.y = (csize.height - new_size.height) / 2;
		}
		else if (q < 1.0)
		{
			new_size.width = static_cast<int>(new_size.width * q);
			aspect_ratio.x = (csize.width - new_size.width) / 2;
		}
	}

	aspect_ratio.size = new_size;

	if (!image_to_flip || aspect_ratio.width < csize.width || aspect_ratio.height < csize.height)
	{
		// Clear the window background to black
		gl_state.clear_color(0, 0, 0, 0);
		gl::screen.clear(gl::buffers::color);
	}

	if (image_to_flip)
	{
		if (m_frame->screenshot_toggle == true)
		{
			m_frame->screenshot_toggle = false;

			std::vector<u8> sshot_frame(buffer_height * buffer_width * 4);

			gl::pixel_pack_settings pack_settings{};
			pack_settings.apply();

			if (gl::get_driver_caps().ARB_dsa_supported)
				glGetTextureImage(image_to_flip, 0, GL_BGRA, GL_UNSIGNED_BYTE, buffer_height * buffer_width * 4, sshot_frame.data());
			else
				glGetTextureImageEXT(image_to_flip, GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, sshot_frame.data());

			if (GLenum err; (err = glGetError()) != GL_NO_ERROR)
				screenshot.error("Failed to capture image: 0x%x", err);
			else
				m_frame->take_screenshot(std::move(sshot_frame), buffer_width, buffer_height);
		}

		areai screen_area = coordi({}, { static_cast<int>(buffer_width), static_cast<int>(buffer_height) });

		if (g_cfg.video.full_rgb_range_output && rsx::fcmp(avconfig->gamma, 1.f))
		{
			// Blit source image to the screen
			m_flip_fbo.recreate();
			m_flip_fbo.bind();
			m_flip_fbo.color = image_to_flip;
			m_flip_fbo.read_buffer(m_flip_fbo.color);
			m_flip_fbo.draw_buffer(m_flip_fbo.color);
			m_flip_fbo.blit(gl::screen, screen_area, areai(aspect_ratio).flipped_vertical(), gl::buffers::color, gl::filter::linear);
		}
		else
		{
			const f32 gamma = avconfig->gamma;
			const bool limited_range = !g_cfg.video.full_rgb_range_output;

			gl::screen.bind();
			m_video_output_pass.run(areau(aspect_ratio), image_to_flip, gamma, limited_range);
		}
	}

	if (m_overlay_manager)
	{
		if (m_overlay_manager->has_dirty())
		{
			m_overlay_manager->lock();

			std::vector<u32> uids_to_dispose;
			uids_to_dispose.reserve(m_overlay_manager->get_dirty().size());

			for (const auto& view : m_overlay_manager->get_dirty())
			{
				m_ui_renderer.remove_temp_resources(view->uid);
				uids_to_dispose.push_back(view->uid);
			}

			m_overlay_manager->unlock();
			m_overlay_manager->dispose(uids_to_dispose);
		}

		if (m_overlay_manager->has_visible())
		{
			gl::screen.bind();

			// Lock to avoid modification during run-update chain
			std::lock_guard lock(*m_overlay_manager);

			for (const auto& view : m_overlay_manager->get_views())
			{
				m_ui_renderer.run(areau(aspect_ratio), 0, *view.get());
			}
		}
	}

	if (g_cfg.video.overlay)
	{
		gl::screen.bind();
		glViewport(0, 0, m_frame->client_width(), m_frame->client_height());

		m_text_printer.print_text(0,  0, m_frame->client_width(), m_frame->client_height(), fmt::format("RSX Load:                %3d%%", get_load()));
		m_text_printer.print_text(0, 18, m_frame->client_width(), m_frame->client_height(), fmt::format("draw calls: %16d", info.stats.draw_calls));
		m_text_printer.print_text(0, 36, m_frame->client_width(), m_frame->client_height(), fmt::format("draw call setup: %11dus", info.stats.setup_time));
		m_text_printer.print_text(0, 54, m_frame->client_width(), m_frame->client_height(), fmt::format("vertex upload time: %8dus", info.stats.vertex_upload_time));
		m_text_printer.print_text(0, 72, m_frame->client_width(), m_frame->client_height(), fmt::format("textures upload time: %6dus", info.stats.textures_upload_time));
		m_text_printer.print_text(0, 90, m_frame->client_width(), m_frame->client_height(), fmt::format("draw call execution: %7dus", info.stats.draw_exec_time));

		const auto num_dirty_textures = m_gl_texture_cache.get_unreleased_textures_count();
		const auto texture_memory_size = m_gl_texture_cache.get_texture_memory_in_use() / (1024 * 1024);
		const auto num_flushes = m_gl_texture_cache.get_num_flush_requests();
		const auto num_mispredict = m_gl_texture_cache.get_num_cache_mispredictions();
		const auto num_speculate = m_gl_texture_cache.get_num_cache_speculative_writes();
		const auto num_misses = m_gl_texture_cache.get_num_cache_misses();
		const auto num_unavoidable = m_gl_texture_cache.get_num_unavoidable_hard_faults();
		const auto cache_miss_ratio = static_cast<u32>(ceil(m_gl_texture_cache.get_cache_miss_ratio() * 100));
		m_text_printer.print_text(0, 126, m_frame->client_width(), m_frame->client_height(), fmt::format("Unreleased textures: %7d", num_dirty_textures));
		m_text_printer.print_text(0, 144, m_frame->client_width(), m_frame->client_height(), fmt::format("Texture memory: %12dM", texture_memory_size));
		m_text_printer.print_text(0, 162, m_frame->client_width(), m_frame->client_height(), fmt::format("Flush requests: %12d  = %2d (%3d%%) hard faults, %2d unavoidable, %2d misprediction(s), %2d speculation(s)", num_flushes, num_misses, cache_miss_ratio, num_unavoidable, num_mispredict, num_speculate));
	}

	m_frame->flip(m_context);
	rsx::thread::flip(info);

	// Cleanup
	m_gl_texture_cache.on_frame_end();
	m_vertex_cache->purge();

	auto removed_textures = m_rtts.free_invalidated();
	m_framebuffer_cache.remove_if([&](auto& fbo)
	{
		if (fbo.unused_check_count() >= 2) return true; // Remove if stale
		if (fbo.references_any(removed_textures)) return true; // Remove if any of the attachments is invalid

		return false;
	});

	if (m_draw_fbo && !m_rtts_dirty)
	{
		// Always restore the active framebuffer
		m_draw_fbo->bind();
		set_viewport();
		set_scissor(!!(m_graphics_state & rsx::pipeline_state::scissor_setup_clipped));
	}
}
