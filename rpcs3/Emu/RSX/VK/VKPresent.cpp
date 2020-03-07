#include "stdafx.h"
#include "VKGSRender.h"


void VKGSRender::reinitialize_swapchain()
{
	m_swapchain_dims.width = m_frame->client_width();
	m_swapchain_dims.height = m_frame->client_height();

	// Reject requests to acquire new swapchain if the window is minimized
	// The NVIDIA driver will spam VK_ERROR_OUT_OF_DATE_KHR if you try to acquire an image from the swapchain and the window is minimized
	// However, any attempt to actually renew the swapchain will crash the driver with VK_ERROR_DEVICE_LOST while the window is in this state
	if (m_swapchain_dims.width == 0 || m_swapchain_dims.height == 0)
	{
		swapchain_unavailable = true;
		return;
	}

	// NOTE: This operation will create a hard sync point
	close_and_submit_command_buffer(m_current_command_buffer->submit_fence);
	m_current_command_buffer->pending = true;
	m_current_command_buffer->reset();

	for (auto &ctx : frame_context_storage)
	{
		if (ctx.present_image == UINT32_MAX)
			continue;

		// Release present image by presenting it
		frame_context_cleanup(&ctx, true);
	}

	// Drain all the queues
	vkDeviceWaitIdle(*m_device);

	// Rebuild swapchain. Old swapchain destruction is handled by the init_swapchain call
	if (!m_swapchain->init(m_swapchain_dims.width, m_swapchain_dims.height))
	{
		rsx_log.warning("Swapchain initialization failed. Request ignored [%dx%d]", m_swapchain_dims.width, m_swapchain_dims.height);
		swapchain_unavailable = true;
		open_command_buffer();
		return;
	}

	// Prepare new swapchain images for use
	open_command_buffer();

	for (u32 i = 0; i < m_swapchain->get_swap_image_count(); ++i)
	{
		const auto target_layout = m_swapchain->get_optimal_present_layout();
		const auto target_image = m_swapchain->get_image(i);
		VkClearColorValue clear_color{};
		VkImageSubresourceRange range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

		vk::change_image_layout(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
		vkCmdClearColorImage(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &range);
		vk::change_image_layout(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, target_layout, range);
	}

	// Will have to block until rendering is completed
	vk::fence resize_fence(*m_device);

	// Flush the command buffer
	close_and_submit_command_buffer(&resize_fence);
	vk::wait_for_fence(&resize_fence);

	m_current_command_buffer->reset();
	open_command_buffer();

	swapchain_unavailable = false;
	should_reinitialize_swapchain = false;
}

void VKGSRender::present(vk::frame_context_t *ctx)
{
	verify(HERE), ctx->present_image != UINT32_MAX;

	// Partial CS flush
	ctx->swap_command_buffer->flush();

	if (!swapchain_unavailable)
	{
		switch (VkResult error = m_swapchain->present(ctx->present_wait_semaphore, ctx->present_image))
		{
		case VK_SUCCESS:
			break;
		case VK_SUBOPTIMAL_KHR:
			should_reinitialize_swapchain = true;
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			swapchain_unavailable = true;
			break;
		default:
			vk::die_with_error(HERE, error);
		}
	}

	// Presentation image released; reset value
	ctx->present_image = UINT32_MAX;
}

void VKGSRender::advance_queued_frames()
{
	// Check all other frames for completion and clear resources
	check_present_status();

	// m_rtts storage is double buffered and should be safe to tag on frame boundary
	m_rtts.free_invalidated();

	// Texture cache is also double buffered to prevent use-after-free
	m_texture_cache.on_frame_end();
	m_samplers_dirty.store(true);

	vk::remove_unused_framebuffers();

	m_vertex_cache->purge();
	m_current_frame->tag_frame_end(m_attrib_ring_info.get_current_put_pos_minus_one(),
		m_vertex_env_ring_info.get_current_put_pos_minus_one(),
		m_fragment_env_ring_info.get_current_put_pos_minus_one(),
		m_vertex_layout_ring_info.get_current_put_pos_minus_one(),
		m_fragment_texture_params_ring_info.get_current_put_pos_minus_one(),
		m_fragment_constants_ring_info.get_current_put_pos_minus_one(),
		m_transform_constants_ring_info.get_current_put_pos_minus_one(),
		m_index_buffer_ring_info.get_current_put_pos_minus_one(),
		m_texture_upload_buffer_ring_info.get_current_put_pos_minus_one());

	m_queued_frames.push_back(m_current_frame);
	verify(HERE), m_queued_frames.size() <= VK_MAX_ASYNC_FRAMES;

	m_current_queue_index = (m_current_queue_index + 1) % VK_MAX_ASYNC_FRAMES;
	m_current_frame = &frame_context_storage[m_current_queue_index];
	m_current_frame->flags |= frame_context_state::dirty;

	vk::advance_frame_counter();
}

void VKGSRender::queue_swap_request()
{
	verify(HERE), !m_current_frame->swap_command_buffer;
	m_current_frame->swap_command_buffer = m_current_command_buffer;

	if (m_swapchain->is_headless())
	{
		m_swapchain->end_frame(*m_current_command_buffer, m_current_frame->present_image);
		close_and_submit_command_buffer(m_current_command_buffer->submit_fence);
	}
	else
	{
		close_and_submit_command_buffer(m_current_command_buffer->submit_fence,
			m_current_frame->acquire_signal_semaphore,
			m_current_frame->present_wait_semaphore,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT);
	}

	// Signal pending state as the command queue is now closed
	m_current_frame->swap_command_buffer->pending = true;

	// Set up a present request for this frame as well
	present(m_current_frame);

	// Grab next cb in line and make it usable
	m_current_cb_index = (m_current_cb_index + 1) % VK_MAX_ASYNC_CB_COUNT;
	m_current_command_buffer = &m_primary_cb_list[m_current_cb_index];
	m_current_command_buffer->reset();

	// Set up new pointers for the next frame
	advance_queued_frames();
	open_command_buffer();
}

void VKGSRender::frame_context_cleanup(vk::frame_context_t *ctx, bool free_resources)
{
	verify(HERE), ctx->swap_command_buffer;

	if (ctx->swap_command_buffer->pending)
	{
		// Perform hard swap here
		if (ctx->swap_command_buffer->wait(FRAME_PRESENT_TIMEOUT) != VK_SUCCESS)
		{
			// Lost surface/device, release swapchain
			swapchain_unavailable = true;
		}

		free_resources = true;
	}

	if (free_resources)
	{
		if (g_cfg.video.overlay)
		{
			m_text_writer->reset_descriptors();
		}

		if (m_overlay_manager && m_overlay_manager->has_dirty())
		{
			m_overlay_manager->lock();

			std::vector<u32> uids_to_dispose;
			uids_to_dispose.reserve(m_overlay_manager->get_dirty().size());

			for (const auto& view : m_overlay_manager->get_dirty())
			{
				m_ui_renderer->remove_temp_resources(view->uid);
				uids_to_dispose.push_back(view->uid);
			}

			m_overlay_manager->unlock();
			m_overlay_manager->dispose(uids_to_dispose);
		}

		vk::reset_global_resources();

		m_attachment_clear_pass->free_resources();
		m_depth_converter->free_resources();
		m_ui_renderer->free_resources();
		m_video_output_pass->free_resources();

		ctx->buffer_views_to_clean.clear();

		if (ctx->last_frame_sync_time > m_last_heap_sync_time)
		{
			m_last_heap_sync_time = ctx->last_frame_sync_time;

			// Heap cleanup; deallocates memory consumed by the frame if it is still held
			m_attrib_ring_info.m_get_pos = ctx->attrib_heap_ptr;
			m_vertex_env_ring_info.m_get_pos = ctx->vtx_env_heap_ptr;
			m_fragment_env_ring_info.m_get_pos = ctx->frag_env_heap_ptr;
			m_fragment_constants_ring_info.m_get_pos = ctx->frag_const_heap_ptr;
			m_transform_constants_ring_info.m_get_pos = ctx->vtx_const_heap_ptr;
			m_vertex_layout_ring_info.m_get_pos = ctx->vtx_layout_heap_ptr;
			m_fragment_texture_params_ring_info.m_get_pos = ctx->frag_texparam_heap_ptr;
			m_index_buffer_ring_info.m_get_pos = ctx->index_heap_ptr;
			m_texture_upload_buffer_ring_info.m_get_pos = ctx->texture_upload_heap_ptr;

			m_attrib_ring_info.notify();
			m_vertex_env_ring_info.notify();
			m_fragment_env_ring_info.notify();
			m_fragment_constants_ring_info.notify();
			m_transform_constants_ring_info.notify();
			m_vertex_layout_ring_info.notify();
			m_fragment_texture_params_ring_info.notify();
			m_index_buffer_ring_info.notify();
			m_texture_upload_buffer_ring_info.notify();
		}
	}

	ctx->swap_command_buffer = nullptr;

	// Remove from queued list
	while (!m_queued_frames.empty())
	{
		auto frame = m_queued_frames.front();
		m_queued_frames.pop_front();

		if (frame == ctx)
		{
			break;
		}
	}

	vk::advance_completed_frame_counter();
}

vk::image* VKGSRender::get_present_source(vk::present_surface_info* info, const rsx::avconf* avconfig)
{
	vk::image* image_to_flip = nullptr;

	// Check the surface store first
	const auto format_bpp = get_format_block_size_in_bytes(info->format);
	const auto overlap_info = m_rtts.get_merged_texture_memory_region(*m_current_command_buffer,
		info->address, info->width, info->height, info->pitch, format_bpp, rsx::surface_access::read);

	if (!overlap_info.empty())
	{
		const auto& section = overlap_info.back();
		auto surface = vk::as_rtt(section.surface);
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
				surface->read_barrier(*m_current_command_buffer);
				image_to_flip = section.surface->get_surface(rsx::surface_access::read);

				info->width = rsx::apply_resolution_scale(std::min(surface_width, static_cast<u16>(info->width)), true);
				info->height = rsx::apply_resolution_scale(std::min(surface_height, static_cast<u16>(info->height)), true);
			}
		}
	}
	else if (auto surface = m_texture_cache.find_texture_from_dimensions<true>(info->address, info->format);
			 surface && surface->get_width() >= info->width && surface->get_height() >= info->height)
	{
		// Hack - this should be the first location to check for output
		// The render might have been done offscreen or in software and a blit used to display
		image_to_flip = surface->get_raw_texture();
	}

	if (!image_to_flip)
	{
		// Read from cell
		const auto range = utils::address_range::start_length(info->address, info->pitch * info->height);
		const u32  lookup_mask = rsx::texture_upload_context::blit_engine_dst | rsx::texture_upload_context::framebuffer_storage;
		const auto overlap = m_texture_cache.find_texture_from_range<true>(range, 0, lookup_mask);

		for (const auto & section : overlap)
		{
			if (!section->is_synchronized())
			{
				section->copy_texture(*m_current_command_buffer, true);
			}
		}

		if (m_current_command_buffer->flags & vk::command_buffer::cb_has_dma_transfer)
		{
			// Submit for processing to lower hard fault penalty
			flush_command_queue();
		}

		m_texture_cache.invalidate_range(*m_current_command_buffer, range, rsx::invalidation_cause::read);
		image_to_flip = m_texture_cache.upload_image_simple(*m_current_command_buffer, info->address, info->width, info->height, info->pitch);
	}

	return image_to_flip;
}

void VKGSRender::flip(const rsx::display_flip_info_t& info)
{
	// Check swapchain condition/status
	if (!m_swapchain->supports_automatic_wm_reports())
	{
		if (m_swapchain_dims.width != m_frame->client_width() + 0u ||
			m_swapchain_dims.height != m_frame->client_height() + 0u)
		{
			swapchain_unavailable = true;
		}
	}

	if (swapchain_unavailable || should_reinitialize_swapchain)
	{
		reinitialize_swapchain();
	}

	m_profiler.start();

	if (m_current_frame == &m_aux_frame_context)
	{
		m_current_frame = &frame_context_storage[m_current_queue_index];
		if (m_current_frame->swap_command_buffer)
		{
			// Its possible this flip request is triggered by overlays and the flip queue is in undefined state
			frame_context_cleanup(m_current_frame, true);
		}

		// Swap aux storage and current frame; aux storage should always be ready for use at all times
		m_current_frame->swap_storage(m_aux_frame_context);
		m_current_frame->grab_resources(m_aux_frame_context);
	}
	else if (m_current_frame->swap_command_buffer)
	{
		if (info.stats.draw_calls > 0)
		{
			// This can be 'legal' if the window was being resized and no polling happened because of swapchain_unavailable flag
			rsx_log.error("Possible data corruption on frame context storage detected");
		}

		// There were no draws and back-to-back flips happened
		frame_context_cleanup(m_current_frame, true);
	}

	if (info.skip_frame || swapchain_unavailable)
	{
		if (!info.skip_frame)
		{
			verify(HERE), swapchain_unavailable;

			// Perform a mini-flip here without invoking present code
			m_current_frame->swap_command_buffer = m_current_command_buffer;
			flush_command_queue(true);
			vk::advance_frame_counter();
			frame_context_cleanup(m_current_frame, true);
		}

		m_frame->flip(m_context);
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

		const u32 video_frame_height = (!avconfig->_3d? avconfig->resolution_y : (avconfig->resolution_y - 30) / 2);
		buffer_width = std::min(buffer_width, avconfig->resolution_x);
		buffer_height = std::min(buffer_height, video_frame_height);
	}
	else
	{
		av_format = CELL_GCM_TEXTURE_A8R8G8B8;
		if (!buffer_pitch)
			buffer_pitch = buffer_width * 4;
	}

	// Scan memory for required data. This is done early to optimize waiting for the driver image acquire below.
	vk::image *image_to_flip = nullptr, *image_to_flip2 = nullptr;
	if (info.buffer < display_buffers_count && buffer_width && buffer_height)
	{
		vk::present_surface_info present_info;
		present_info.width = buffer_width;
		present_info.height = buffer_height;
		present_info.pitch = buffer_pitch;
		present_info.format = av_format;
		present_info.address = rsx::get_address(display_buffers[info.buffer].offset, CELL_GCM_LOCATION_LOCAL, HERE);

		image_to_flip = get_present_source(&present_info, avconfig);

		if (avconfig->_3d) [[unlikely]]
		{
			const auto min_expected_height = rsx::apply_resolution_scale(buffer_height + 30, true);
			if (image_to_flip->height() < min_expected_height)
			{
				// Get image for second eye
				const u32 image_offset = (buffer_height + 30) * buffer_pitch + display_buffers[info.buffer].offset;
				present_info.width = buffer_width;
				present_info.height = buffer_height;
				present_info.address = rsx::get_address(image_offset, CELL_GCM_LOCATION_LOCAL, HERE);

				image_to_flip2 = get_present_source(&present_info, avconfig);
			}
			else
			{
				// Account for possible insets
				buffer_height = std::min<u32>(image_to_flip->height() - min_expected_height, rsx::apply_resolution_scale(buffer_height, true));
			}
		}

		buffer_width = present_info.width;
		buffer_height = present_info.height;
	}

	// Prepare surface for new frame. Set no timeout here so that we wait for the next image if need be
	verify(HERE), m_current_frame->present_image == UINT32_MAX;
	verify(HERE), m_current_frame->swap_command_buffer == nullptr;

	u64 timeout = m_swapchain->get_swap_image_count() <= VK_MAX_ASYNC_FRAMES? 0ull: 100000000ull;
	while (VkResult status = m_swapchain->acquire_next_swapchain_image(m_current_frame->acquire_signal_semaphore, timeout, &m_current_frame->present_image))
	{
		switch (status)
		{
		case VK_TIMEOUT:
		case VK_NOT_READY:
		{
			// In some cases, after a fullscreen switch, the driver only allows N-1 images to be acquirable, where N = number of available swap images.
			// This means that any acquired images have to be released
			// before acquireNextImage can return successfully. This is despite the driver reporting 2 swap chain images available
			// This makes fullscreen performance slower than windowed performance as throughput is lowered due to losing one presentable image
			// Found on AMD Crimson 17.7.2


			// Whatever returned from status, this is now a spin
			timeout = 0ull;
			check_present_status();
			continue;
		}
		case VK_SUBOPTIMAL_KHR:
			should_reinitialize_swapchain = true;
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			rsx_log.warning("vkAcquireNextImageKHR failed with VK_ERROR_OUT_OF_DATE_KHR. Flip request ignored until surface is recreated.");
			swapchain_unavailable = true;
			reinitialize_swapchain();
			continue;
		default:
			vk::die_with_error(HERE, status);
		}

		if (should_reinitialize_swapchain)
		{
			// Image is valid, new swapchain will be generated later
			break;
		}
	}

	// Confirm that the driver did not silently fail
	verify(HERE), m_current_frame->present_image != UINT32_MAX;

	// Calculate output dimensions. Done after swapchain acquisition in case it was recreated.
	coordi aspect_ratio;
	sizei csize = static_cast<sizei>(m_swapchain_dims);
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

	// Blit contents to screen..
	VkImage target_image = m_swapchain->get_image(m_current_frame->present_image);
	const auto present_layout = m_swapchain->get_optimal_present_layout();

	const VkImageSubresourceRange subresource_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	VkImageLayout target_layout = present_layout;

	VkRenderPass single_target_pass = VK_NULL_HANDLE;
	vk::framebuffer_holder* direct_fbo = nullptr;
	rsx::simple_array<vk::viewable_image*> calibration_src;

	if (!image_to_flip || aspect_ratio.width < csize.width || aspect_ratio.height < csize.height)
	{
		// Clear the window background to black
		VkClearColorValue clear_black {};
		vk::change_image_layout(*m_current_command_buffer, target_image, present_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);
		vkCmdClearColorImage(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_black, 1, &subresource_range);

		target_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}

	if (image_to_flip)
	{
		if (!g_cfg.video.full_rgb_range_output || !rsx::fcmp(avconfig->gamma, 1.f) || avconfig->_3d) [[unlikely]]
		{
			calibration_src.push_back(dynamic_cast<vk::viewable_image*>(image_to_flip));
			verify("Image not viewable" HERE), calibration_src.front();

			if (image_to_flip2)
			{
				calibration_src.push_back(dynamic_cast<vk::viewable_image*>(image_to_flip2));
				verify("Image not viewable" HERE), calibration_src.back();
			}
		}

		if (calibration_src.empty()) [[likely]]
		{
			vk::copy_scaled_image(*m_current_command_buffer, image_to_flip->value, target_image, image_to_flip->current_layout, target_layout,
				{ 0, 0, static_cast<s32>(buffer_width), static_cast<s32>(buffer_height) }, aspect_ratio, 1, VK_IMAGE_ASPECT_COLOR_BIT, false);
		}
		else
		{
			vk::change_image_layout(*m_current_command_buffer, target_image, target_layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, subresource_range);
			target_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			const auto key = vk::get_renderpass_key(m_swapchain->get_surface_format());
			single_target_pass = vk::get_renderpass(*m_device, key);
			verify("Usupported renderpass configuration" HERE), single_target_pass != VK_NULL_HANDLE;

			direct_fbo = vk::get_framebuffer(*m_device, m_swapchain_dims.width, m_swapchain_dims.height, single_target_pass, m_swapchain->get_surface_format(), target_image);
			direct_fbo->add_ref();

			image_to_flip->push_layout(*m_current_command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			m_video_output_pass->run(*m_current_command_buffer, areau(aspect_ratio), direct_fbo, calibration_src, avconfig->gamma, !g_cfg.video.full_rgb_range_output, avconfig->_3d, single_target_pass);
			image_to_flip->pop_layout(*m_current_command_buffer);

			direct_fbo->release();
		}

		if (m_frame->screenshot_toggle == true)
		{
			m_frame->screenshot_toggle = false;

			const size_t sshot_size = buffer_height * buffer_width * 4;

			vk::buffer sshot_vkbuf(*m_device, align(sshot_size, 0x100000), m_device->get_memory_mapping().host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0);

			VkBufferImageCopy copy_info;
			copy_info.bufferOffset                    = 0;
			copy_info.bufferRowLength                 = 0;
			copy_info.bufferImageHeight               = 0;
			copy_info.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			copy_info.imageSubresource.baseArrayLayer = 0;
			copy_info.imageSubresource.layerCount     = 1;
			copy_info.imageSubresource.mipLevel       = 0;
			copy_info.imageOffset.x                   = 0;
			copy_info.imageOffset.y                   = 0;
			copy_info.imageOffset.z                   = 0;
			copy_info.imageExtent.width               = buffer_width;
			copy_info.imageExtent.height              = buffer_height;
			copy_info.imageExtent.depth               = 1;

			image_to_flip->push_layout(*m_current_command_buffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			vk::copy_image_to_buffer(*m_current_command_buffer, image_to_flip, &sshot_vkbuf, copy_info);
			image_to_flip->pop_layout(*m_current_command_buffer);

			flush_command_queue(true);
			auto src = sshot_vkbuf.map(0, sshot_size);
			std::vector<u8> sshot_frame(sshot_size);
			memcpy(sshot_frame.data(), src, sshot_size);
			sshot_vkbuf.unmap();

			m_frame->take_screenshot(std::move(sshot_frame), buffer_width, buffer_height);
		}
	}

	const bool has_overlay = (m_overlay_manager && m_overlay_manager->has_visible());
	if (g_cfg.video.overlay || has_overlay)
	{
		if (target_layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			// Change the image layout whilst setting up a dependency on waiting for the blit op to finish before we start writing
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.oldLayout = target_layout;
			barrier.image = target_image;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange = subresource_range;
			vkCmdPipelineBarrier(*m_current_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);

			target_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}

		if (!direct_fbo)
		{
			const auto key = vk::get_renderpass_key(m_swapchain->get_surface_format());
			single_target_pass = vk::get_renderpass(*m_device, key);
			verify("Usupported renderpass configuration" HERE), single_target_pass != VK_NULL_HANDLE;

			direct_fbo = vk::get_framebuffer(*m_device, m_swapchain_dims.width, m_swapchain_dims.height, single_target_pass, m_swapchain->get_surface_format(), target_image);
		}

		direct_fbo->add_ref();

		if (has_overlay)
		{
			// Lock to avoid modification during run-update chain
			std::lock_guard lock(*m_overlay_manager);

			for (const auto& view : m_overlay_manager->get_views())
			{
				m_ui_renderer->run(*m_current_command_buffer, areau(aspect_ratio), direct_fbo, single_target_pass, m_texture_upload_buffer_ring_info, *view.get());
			}
		}

		if (g_cfg.video.overlay)
		{
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0,   0, direct_fbo->width(), direct_fbo->height(), fmt::format("RSX Load:                 %3d%%", get_load()));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0,  18, direct_fbo->width(), direct_fbo->height(), fmt::format("draw calls: %17d", info.stats.draw_calls));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0,  36, direct_fbo->width(), direct_fbo->height(), fmt::format("draw call setup: %12dus", info.stats.setup_time));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0,  54, direct_fbo->width(), direct_fbo->height(), fmt::format("vertex upload time: %9dus", info.stats.vertex_upload_time));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0,  72, direct_fbo->width(), direct_fbo->height(), fmt::format("texture upload time: %8dus", info.stats.textures_upload_time));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0,  90, direct_fbo->width(), direct_fbo->height(), fmt::format("draw call execution: %8dus", info.stats.draw_exec_time));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 108, direct_fbo->width(), direct_fbo->height(), fmt::format("submit and flip: %12dus", info.stats.flip_time));

			const auto num_dirty_textures = m_texture_cache.get_unreleased_textures_count();
			const auto texture_memory_size = m_texture_cache.get_texture_memory_in_use() / (1024 * 1024);
			const auto tmp_texture_memory_size = m_texture_cache.get_temporary_memory_in_use() / (1024 * 1024);
			const auto num_flushes = m_texture_cache.get_num_flush_requests();
			const auto num_mispredict = m_texture_cache.get_num_cache_mispredictions();
			const auto num_speculate = m_texture_cache.get_num_cache_speculative_writes();
			const auto num_misses = m_texture_cache.get_num_cache_misses();
			const auto num_unavoidable = m_texture_cache.get_num_unavoidable_hard_faults();
			const auto cache_miss_ratio = static_cast<u32>(ceil(m_texture_cache.get_cache_miss_ratio() * 100));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 144, direct_fbo->width(), direct_fbo->height(), fmt::format("Unreleased textures: %8d", num_dirty_textures));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 162, direct_fbo->width(), direct_fbo->height(), fmt::format("Texture cache memory: %7dM", texture_memory_size));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 180, direct_fbo->width(), direct_fbo->height(), fmt::format("Temporary texture memory: %3dM", tmp_texture_memory_size));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 198, direct_fbo->width(), direct_fbo->height(), fmt::format("Flush requests: %13d  = %2d (%3d%%) hard faults, %2d unavoidable, %2d misprediction(s), %2d speculation(s)", num_flushes, num_misses, cache_miss_ratio, num_unavoidable, num_mispredict, num_speculate));
		}

		direct_fbo->release();
	}

	if (target_layout != present_layout)
	{
		vk::change_image_layout(*m_current_command_buffer, target_image, target_layout, present_layout, subresource_range);
	}

	queue_swap_request();

	m_frame_stats.flip_time = m_profiler.duration();

	m_frame->flip(m_context);
	rsx::thread::flip(info);
}
