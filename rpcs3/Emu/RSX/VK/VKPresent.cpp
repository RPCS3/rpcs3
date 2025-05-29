#include "stdafx.h"
#include "VKGSRender.h"
#include "vkutils/buffer_object.h"
#include "Emu/RSX/Overlays/overlay_manager.h"
#include "Emu/RSX/Overlays/overlay_debug_overlay.h"
#include "Emu/Cell/Modules/cellVideoOut.h"

#include "upscalers/bilinear_pass.hpp"
#include "upscalers/fsr_pass.h"
#include "upscalers/nearest_pass.hpp"
#include "util/asm.hpp"
#include "util/video_provider.h"

extern atomic_t<bool> g_user_asked_for_screenshot;
extern atomic_t<recording_mode> g_recording_mode;

namespace
{
	VkFormat RSX_display_format_to_vk_format(u8 format)
	{
		switch (format)
		{
		default:
			rsx_log.error("Unhandled video output format 0x%x", static_cast<s32>(format));
			[[fallthrough]];
		case CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8:
			return VK_FORMAT_B8G8R8A8_UNORM;
		case CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8B8G8R8:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_R16G16B16X16_FLOAT:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		}
	}
}

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
	close_and_submit_command_buffer();
	m_current_command_buffer->reset();
	m_current_command_buffer->begin();

	for (auto &ctx : frame_context_storage)
	{
		if (ctx.present_image == umax)
			continue;

		// Release present image by presenting it
		frame_context_cleanup(&ctx);
	}

	// Discard the current upscaling pipeline if any
	m_upscaler.reset();

	// Drain all the queues
	vkDeviceWaitIdle(*m_device);

	// Rebuild swapchain. Old swapchain destruction is handled by the init_swapchain call
	if (!m_swapchain->init(m_swapchain_dims.width, m_swapchain_dims.height))
	{
		rsx_log.warning("Swapchain initialization failed. Request ignored [%dx%d]", m_swapchain_dims.width, m_swapchain_dims.height);
		swapchain_unavailable = true;
		return;
	}

	// Prepare new swapchain images for use
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
	m_current_command_buffer->begin();

	swapchain_unavailable = false;
	should_reinitialize_swapchain = false;
}

void VKGSRender::present(vk::frame_context_t *ctx)
{
	ensure(ctx->present_image != umax);

	// Partial CS flush
	ctx->swap_command_buffer->flush();

	if (!swapchain_unavailable)
	{
		switch (VkResult error = m_swapchain->present(ctx->present_wait_semaphore, ctx->present_image))
		{
		case VK_SUCCESS:
			break;
		case VK_SUBOPTIMAL_KHR:
#ifndef ANDROID
			should_reinitialize_swapchain = true;
#endif
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			swapchain_unavailable = true;
			break;
		default:
			// Other errors not part of rpcs3. This can be caused by 3rd party injectors with bad code, of which we have no control over.
			// Let the application attempt to recover instead of crashing outright.
			rsx_log.error("VkPresent returned unexpected error code %lld. Will attempt to recreate the swapchain. Please disable 3rd party injector tools.", static_cast<s64>(error));
			swapchain_unavailable = true;
			break;
		}
	}

	// Presentation image released; reset value
	ctx->present_image = -1;
}

void VKGSRender::advance_queued_frames()
{
	// Check all other frames for completion and clear resources
	check_present_status();

	// Run video memory balancer
	m_device->rebalance_memory_type_usage();
	vk::vmm_check_memory_usage();

	// m_rtts storage is double buffered and should be safe to tag on frame boundary
	m_rtts.trim(*m_current_command_buffer, vk::vmm_determine_memory_load_severity());

	// Texture cache is also double buffered to prevent use-after-free
	m_texture_cache.on_frame_end();
	m_samplers_dirty.store(true);

	vk::remove_unused_framebuffers();

	m_vertex_cache->purge();
	m_current_frame->tag_frame_end();

	m_queued_frames.push_back(m_current_frame);
	ensure(m_queued_frames.size() <= VK_MAX_ASYNC_FRAMES);

	m_current_queue_index = (m_current_queue_index + 1) % VK_MAX_ASYNC_FRAMES;
	m_current_frame = &frame_context_storage[m_current_queue_index];
	m_current_frame->flags |= frame_context_state::dirty;

	vk::advance_frame_counter();
}

void VKGSRender::queue_swap_request()
{
	ensure(!m_current_frame->swap_command_buffer);
	m_current_frame->swap_command_buffer = m_current_command_buffer;

	if (m_swapchain->is_headless())
	{
		m_swapchain->end_frame(*m_current_command_buffer, m_current_frame->present_image);
		close_and_submit_command_buffer();
	}
	else
	{
		close_and_submit_command_buffer(nullptr,
			m_current_frame->acquire_signal_semaphore,
			m_current_frame->present_wait_semaphore,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT);
	}

	// Set up a present request for this frame as well
	present(m_current_frame);

	// Grab next cb in line and make it usable
	m_current_command_buffer = m_primary_cb_list.next();
	m_current_command_buffer->reset();
	m_current_command_buffer->begin();

	// Set up new pointers for the next frame
	advance_queued_frames();
}

void VKGSRender::frame_context_cleanup(vk::frame_context_t *ctx)
{
	ensure(ctx->swap_command_buffer);

	// Perform hard swap here
	if (ctx->swap_command_buffer->wait(FRAME_PRESENT_TIMEOUT) != VK_SUCCESS)
	{
		// Lost surface/device, release swapchain
		swapchain_unavailable = true;
	}

	// Resource cleanup.
	{
		if (m_overlay_manager && m_overlay_manager->has_dirty())
		{
			auto ui_renderer = vk::get_overlay_pass<vk::ui_overlay_renderer>();
			m_overlay_manager->lock_shared();

			std::vector<u32> uids_to_dispose;
			uids_to_dispose.reserve(m_overlay_manager->get_dirty().size());

			for (const auto& view : m_overlay_manager->get_dirty())
			{
				ui_renderer->remove_temp_resources(view->uid);
				uids_to_dispose.push_back(view->uid);
			}

			m_overlay_manager->unlock_shared();
			m_overlay_manager->dispose(uids_to_dispose);
		}

		vk::get_resource_manager()->trim();

		vk::reset_global_resources();

		if (ctx->last_frame_sync_time > m_last_heap_sync_time)
		{
			m_last_heap_sync_time = ctx->last_frame_sync_time;

			// Heap cleanup; deallocates memory consumed by the frame if it is still held
			vk::data_heap_manager::restore_snapshot(ctx->heap_snapshot);
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

vk::viewable_image* VKGSRender::get_present_source(/* inout */ vk::present_surface_info* info, const rsx::avconf& avconfig)
{
	vk::viewable_image* image_to_flip = nullptr;

	// @FIXME: This entire function needs to be rewritten to go through the texture cache's "upload_texture" routine.
	// That method is not a 1:1 replacement due to handling of insets that is done differently here.

	// Check the surface store first
	const auto format_bpp = rsx::get_format_block_size_in_bytes(info->format);
	const auto overlap_info = m_rtts.get_merged_texture_memory_region(*m_current_command_buffer,
		info->address, info->width, info->height, info->pitch, format_bpp, rsx::surface_access::transfer_read);

	if (!overlap_info.empty())
	{
		const auto& section = overlap_info.back();
		auto surface = vk::as_rtt(section.surface);
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
				image_to_flip = section.surface->get_surface(rsx::surface_access::transfer_read);

				std::tie(info->width, info->height) = rsx::apply_resolution_scale<true>(
					std::min(surface_width, info->width),
					std::min(surface_height, info->height));
			}
		}
	}
	else if (auto surface = m_texture_cache.find_texture_from_dimensions<true>(info->address, info->format);
			 surface && surface->get_width() >= info->width && surface->get_height() >= info->height)
	{
		// Hack - this should be the first location to check for output
		// The render might have been done offscreen or in software and a blit used to display
		image_to_flip = dynamic_cast<vk::viewable_image*>(surface->get_raw_texture());
	}

	// The correct output format is determined by the AV configuration set in CellVideoOutConfigure by the game.
	// 99.9% of the time, this will match the backbuffer fbo format used in rendering/compositing the output.
	// But in some cases, let's just say some devs are creative.
	const auto expected_format = RSX_display_format_to_vk_format(avconfig.format);

	if (!image_to_flip) [[ unlikely ]]
	{
		// Read from cell
		const auto range = utils::address_range32::start_length(info->address, info->pitch * info->height);
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
		image_to_flip = m_texture_cache.upload_image_simple(*m_current_command_buffer, expected_format, info->address, info->width, info->height, info->pitch);
	}
	else if (image_to_flip->format() != expected_format)
	{
		// Devs are being creative. Force-cast this to the proper pixel layout.
		auto dst_img = m_texture_cache.create_temporary_subresource_storage(
			RSX_FORMAT_CLASS_COLOR, expected_format, info->width, info->height, 1, 1, 1,
			VK_IMAGE_TYPE_2D, 0, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

		if (dst_img)
		{
			const areai src_rect = { 0, 0, static_cast<int>(info->width), static_cast<int>(info->height) };
			const areai dst_rect = src_rect;

			dst_img->change_layout(*m_current_command_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			if (vk::formats_are_bitcast_compatible(dst_img.get(), image_to_flip))
			{
				vk::copy_image(*m_current_command_buffer, image_to_flip, dst_img.get(), src_rect, dst_rect, 1);
			}
			else
			{
				vk::copy_image_typeless(*m_current_command_buffer, image_to_flip, dst_img.get(), src_rect, dst_rect, 1);
			}

			image_to_flip = dst_img.get();
			m_texture_cache.dispose_reusable_image(dst_img);
		}
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
			frame_context_cleanup(m_current_frame);
		}

		// Swap aux storage and current frame; aux storage should always be ready for use at all times
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
		frame_context_cleanup(m_current_frame);
	}

	if (info.skip_frame || swapchain_unavailable)
	{
		if (!info.skip_frame)
		{
			ensure(swapchain_unavailable);

			// Perform a mini-flip here without invoking present code
			m_current_frame->swap_command_buffer = m_current_command_buffer;
			flush_command_queue(true);
			vk::advance_frame_counter();
			frame_context_cleanup(m_current_frame);
		}

		m_frame->flip(m_context);
		rsx::thread::flip(info);
		return;
	}

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

		const u32 video_frame_height = (avconfig.stereo_mode == stereo_render_mode_options::disabled ? avconfig.resolution_y : ((avconfig.resolution_y - 30) / 2));
		buffer_width = std::min(buffer_width, avconfig.resolution_x);
		buffer_height = std::min(buffer_height, video_frame_height);
	}
	else
	{
		av_format = CELL_GCM_TEXTURE_A8R8G8B8;
		if (!buffer_pitch)
			buffer_pitch = buffer_width * 4;
	}

	// Scan memory for required data. This is done early to optimize waiting for the driver image acquire below.
	vk::viewable_image *image_to_flip = nullptr, *image_to_flip2 = nullptr;
	if (info.buffer < display_buffers_count && buffer_width && buffer_height)
	{
		vk::present_surface_info present_info
		{
			.address = rsx::get_address(display_buffers[info.buffer].offset, CELL_GCM_LOCATION_LOCAL),
			.format = av_format,
			.width = buffer_width,
			.height = buffer_height,
			.pitch = buffer_pitch,
			.eye = 0
		};
		image_to_flip = get_present_source(&present_info, avconfig);

		if (avconfig.stereo_mode != stereo_render_mode_options::disabled) [[unlikely]]
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

	// Prepare surface for new frame. Set no timeout here so that we wait for the next image if need be
	ensure(m_current_frame->present_image == umax);
	ensure(m_current_frame->swap_command_buffer == nullptr);

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
			vk::die_with_error(status);
		}

		if (should_reinitialize_swapchain)
		{
			// Image is valid, new swapchain will be generated later
			break;
		}
	}

	// Confirm that the driver did not silently fail
	ensure(m_current_frame->present_image != umax);

	// Calculate output dimensions. Done after swapchain acquisition in case it was recreated.
	areai aspect_ratio;
	if (!g_cfg.video.stretch_to_display_area)
	{
		const auto converted = avconfig.aspect_convert_region({ buffer_width, buffer_height }, m_swapchain_dims);
		aspect_ratio = static_cast<areai>(converted);
	}
	else
	{
		aspect_ratio = { 0, 0, s32(m_swapchain_dims.width), s32(m_swapchain_dims.height) };
	}

	// Blit contents to screen..
	VkImage target_image = m_swapchain->get_image(m_current_frame->present_image);
	const auto present_layout = m_swapchain->get_optimal_present_layout();

	const VkImageSubresourceRange subresource_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	VkImageLayout target_layout = present_layout;

	VkRenderPass single_target_pass = VK_NULL_HANDLE;
	vk::framebuffer_holder* direct_fbo = nullptr;
	rsx::simple_array<vk::viewable_image*> calibration_src;

	if (!image_to_flip || aspect_ratio.x1 || aspect_ratio.y1)
	{
		// Clear the window background to black
		VkClearColorValue clear_black {};
		vk::change_image_layout(*m_current_command_buffer, target_image, present_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);
		vkCmdClearColorImage(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_black, 1, &subresource_range);

		target_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}

	const output_scaling_mode output_scaling = g_cfg.video.output_scaling.get();

	if (!m_upscaler || m_output_scaling != output_scaling)
	{
		m_output_scaling = output_scaling;

		if (m_output_scaling == output_scaling_mode::nearest)
		{
			m_upscaler = std::make_unique<vk::nearest_upscale_pass>();
		}
		else if (m_output_scaling == output_scaling_mode::fsr)
		{
			m_upscaler = std::make_unique<vk::fsr_upscale_pass>();
		}
		else
		{
			m_upscaler = std::make_unique<vk::bilinear_upscale_pass>();
		}
	}

	if (image_to_flip)
	{
		const bool use_full_rgb_range_output = g_cfg.video.full_rgb_range_output.get();

		if (!use_full_rgb_range_output || !rsx::fcmp(avconfig.gamma, 1.f) || avconfig.stereo_mode != stereo_render_mode_options::disabled) [[unlikely]]
		{
			if (image_to_flip) calibration_src.push_back(image_to_flip);
			if (image_to_flip2) calibration_src.push_back(image_to_flip2);

			if (m_output_scaling == output_scaling_mode::fsr && avconfig.stereo_mode == stereo_render_mode_options::disabled) // 3D will be implemented later
			{
				// Run upscaling pass before the rest of the output effects pipeline
				// This can be done with all upscalers but we already get bilinear upscaling for free if we just out the filters directly
				VkImageBlit request = {};
				request.srcSubresource = { image_to_flip->aspect(), 0, 0, 1 };
				request.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
				request.srcOffsets[0] = { 0, 0, 0 };
				request.srcOffsets[1] = { s32(buffer_width), s32(buffer_height), 1 };
				request.dstOffsets[0] = { 0, 0, 0 };
				request.dstOffsets[1] = { aspect_ratio.width(), aspect_ratio.height(), 1 };

				for (unsigned i = 0; i < calibration_src.size(); ++i)
				{
					const rsx::flags32_t mode = (i == 0) ? UPSCALE_LEFT_VIEW : UPSCALE_RIGHT_VIEW;
					calibration_src[i] = m_upscaler->scale_output(*m_current_command_buffer, image_to_flip, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED, request, mode);
				}
			}

			vk::change_image_layout(*m_current_command_buffer, target_image, target_layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, subresource_range);
			target_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			const auto key = vk::get_renderpass_key(m_swapchain->get_surface_format());
			single_target_pass = vk::get_renderpass(*m_device, key);
			ensure(single_target_pass != VK_NULL_HANDLE);

			direct_fbo = vk::get_framebuffer(*m_device, m_swapchain_dims.width, m_swapchain_dims.height, VK_FALSE, single_target_pass, m_swapchain->get_surface_format(), target_image);
			direct_fbo->add_ref();

			vk::get_overlay_pass<vk::video_out_calibration_pass>()->run(
				*m_current_command_buffer, areau(aspect_ratio), direct_fbo, calibration_src,
				avconfig.gamma, !use_full_rgb_range_output, avconfig.stereo_mode, single_target_pass);

			direct_fbo->release();
		}
		else
		{
			// Do raw transfer here as there is no image object associated with textures owned by the driver (TODO)
			VkImageBlit rgn = {};
			rgn.srcSubresource = { image_to_flip->aspect(), 0, 0, 1 };
			rgn.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			rgn.srcOffsets[0] = { 0, 0, 0 };
			rgn.srcOffsets[1] = { s32(buffer_width), s32(buffer_height), 1 };
			rgn.dstOffsets[0] = { aspect_ratio.x1, aspect_ratio.y1, 0 };
			rgn.dstOffsets[1] = { aspect_ratio.x2, aspect_ratio.y2, 1 };

			if (target_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				vk::change_image_layout(*m_current_command_buffer, target_image, target_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);
				target_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			}

			m_upscaler->scale_output(*m_current_command_buffer, image_to_flip, target_image, target_layout, rgn, UPSCALE_AND_COMMIT | UPSCALE_DEFAULT_VIEW);
		}

		if (g_user_asked_for_screenshot || (g_recording_mode != recording_mode::stopped && m_frame->can_consume_frame()))
		{
			const usz sshot_size = buffer_height * buffer_width * 4;

			vk::buffer sshot_vkbuf(*m_device, utils::align(sshot_size, 0x100000), m_device->get_memory_mapping().host_visible_coherent,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0, VMM_ALLOCATION_POOL_UNDEFINED);

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

			const bool is_bgra = image_to_flip->format() == VK_FORMAT_B8G8R8A8_UNORM;

			if (g_user_asked_for_screenshot.exchange(false))
			{
				m_frame->take_screenshot(std::move(sshot_frame), buffer_width, buffer_height, is_bgra);
			}
			else
			{
				m_frame->present_frame(sshot_frame, buffer_width * 4, buffer_width, buffer_height, is_bgra);
			}
		}
	}

	const bool has_overlay = (m_overlay_manager && m_overlay_manager->has_visible());
	if (g_cfg.video.debug_overlay || has_overlay)
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
			ensure(single_target_pass != VK_NULL_HANDLE);

			direct_fbo = vk::get_framebuffer(*m_device, m_swapchain_dims.width, m_swapchain_dims.height, VK_FALSE, single_target_pass, m_swapchain->get_surface_format(), target_image);
		}

		direct_fbo->add_ref();

		if (has_overlay)
		{
			// Lock to avoid modification during run-update chain
			auto ui_renderer = vk::get_overlay_pass<vk::ui_overlay_renderer>();
			std::lock_guard lock(*m_overlay_manager);

			for (const auto& view : m_overlay_manager->get_views())
			{
				ui_renderer->run(*m_current_command_buffer, areau(aspect_ratio), direct_fbo, single_target_pass, m_texture_upload_buffer_ring_info, *view.get());
			}
		}

		if (g_cfg.video.debug_overlay)
		{
			const auto num_dirty_textures = m_texture_cache.get_unreleased_textures_count();
			const auto texture_memory_size = m_texture_cache.get_texture_memory_in_use() / (1024 * 1024);
			const auto tmp_texture_memory_size = m_texture_cache.get_temporary_memory_in_use() / (1024 * 1024);
			const auto num_flushes = m_texture_cache.get_num_flush_requests();
			const auto num_mispredict = m_texture_cache.get_num_cache_mispredictions();
			const auto num_speculate = m_texture_cache.get_num_cache_speculative_writes();
			const auto num_misses = m_texture_cache.get_num_cache_misses();
			const auto num_unavoidable = m_texture_cache.get_num_unavoidable_hard_faults();
			const auto cache_miss_ratio = static_cast<u32>(ceil(m_texture_cache.get_cache_miss_ratio() * 100));
			const auto num_texture_upload = m_texture_cache.get_texture_upload_calls_this_frame();
			const auto num_texture_upload_miss = m_texture_cache.get_texture_upload_misses_this_frame();
			const auto texture_upload_miss_ratio = m_texture_cache.get_texture_upload_miss_percentage();
			const auto texture_copies_ellided = m_texture_cache.get_texture_copies_ellided_this_frame();
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
				"Internal Resolution:      %s\n"
				"RSX Load:                 %3d%%\n"
				"draw calls: %17d\n"
				"submits: %20d\n"
				"draw call setup: %12dus\n"
				"vertex upload time: %9dus\n"
				"texture upload time: %8dus\n"
				"draw call execution: %8dus\n"
				"submit and flip: %12dus\n"
				"Unreleased textures: %8d\n"
				"Texture cache memory: %7dM\n"
				"Temporary texture memory: %3dM\n"
				"Flush requests: %13d  = %2d (%3d%%) hard faults, %2d unavoidable, %2d misprediction(s), %2d speculation(s)\n"
				"Texture uploads: %12u (%u from CPU - %02u%%, %u copies avoided)\n"
				"Vertex cache hits: %10u/%u (%u%%)\n"
				"Program cache lookup ellision: %u/%u (%u%%)",
				info.stats.framebuffer_stats.to_string(!backend_config.supports_hw_msaa),
				get_load(), info.stats.draw_calls, info.stats.submit_count, info.stats.setup_time, info.stats.vertex_upload_time,
				info.stats.textures_upload_time, info.stats.draw_exec_time, info.stats.flip_time,
				num_dirty_textures, texture_memory_size, tmp_texture_memory_size,
				num_flushes, num_misses, cache_miss_ratio, num_unavoidable, num_mispredict, num_speculate,
				num_texture_upload, num_texture_upload_miss, texture_upload_miss_ratio, texture_copies_ellided,
				vertex_cache_hit_count, info.stats.vertex_cache_request_count, vertex_cache_hit_ratio,
				program_cache_ellided, program_cache_lookups, program_cache_ellision_rate)
			);
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
