#include "VKRenderTargets.h"

namespace vk
{
	// Get the linear resolve target bound to this surface. Initialize if none exists
	vk::viewable_image* render_target::get_resolve_target_safe(vk::command_buffer& cmd)
	{
		if (!resolve_surface)
		{
			// Create a resolve surface
			const auto resolve_w = width() * samples_x;
			const auto resolve_h = height() * samples_y;

			VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			usage |= (this->info.usage & (VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT));

			resolve_surface.reset(new vk::viewable_image(
				*g_render_device,
				g_render_device->get_memory_mapping().device_local,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				format(),
				resolve_w, resolve_h, 1, 1, 1,
				VK_SAMPLE_COUNT_1_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL,
				usage,
				0,
				format_class()));

			resolve_surface->native_component_map = native_component_map;
			resolve_surface->change_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
		}

		return resolve_surface.get();
	}

	// Resolve the planar MSAA data into a linear block
	void render_target::resolve(vk::command_buffer& cmd)
	{
		VkImageSubresourceRange range = { aspect(), 0, 1, 0, 1 };

		// NOTE: This surface can only be in the ATTACHMENT_OPTIMAL layout
		// The resolve surface can be in any type of access, but we have to assume it is likely in read-only mode like shader read-only

		if (!is_depth_surface()) [[likely]]
		{
			ensure(current_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			// This is the source; finish writing before reading
			vk::insert_image_memory_barrier(
				cmd, this->value,
				this->current_layout, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				range);

			// This is the target; finish reading before writing
			vk::insert_image_memory_barrier(
				cmd, resolve_surface->value,
				resolve_surface->current_layout, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_SHADER_WRITE_BIT,
				range);

			this->current_layout = VK_IMAGE_LAYOUT_GENERAL;
			resolve_surface->current_layout = VK_IMAGE_LAYOUT_GENERAL;
		}
		else
		{
			this->push_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			resolve_surface->change_layout(cmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		}

		vk::resolve_image(cmd, resolve_surface.get(), this);

		if (!is_depth_surface()) [[likely]]
		{
			vk::insert_image_memory_barrier(
				cmd, this->value,
				this->current_layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				range);

			vk::insert_image_memory_barrier(
				cmd, resolve_surface->value,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
				range);

			this->current_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			resolve_surface->current_layout = VK_IMAGE_LAYOUT_GENERAL;
		}
		else
		{
			this->pop_layout(cmd);
			resolve_surface->change_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
		}

		msaa_flags &= ~(rsx::surface_state_flags::require_resolve);
	}

	// Unresolve the linear data into planar MSAA data
	void render_target::unresolve(vk::command_buffer& cmd)
	{
		ensure(!(msaa_flags & rsx::surface_state_flags::require_resolve));
		VkImageSubresourceRange range = { aspect(), 0, 1, 0, 1 };

		if (!is_depth_surface()) [[likely]]
		{
			ensure(current_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			// This is the dest; finish reading before writing
			vk::insert_image_memory_barrier(
				cmd, this->value,
				this->current_layout, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_SHADER_WRITE_BIT,
				range);

			// This is the source; finish writing before reading
			vk::insert_image_memory_barrier(
				cmd, resolve_surface->value,
				resolve_surface->current_layout, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				range);

			this->current_layout = VK_IMAGE_LAYOUT_GENERAL;
			resolve_surface->current_layout = VK_IMAGE_LAYOUT_GENERAL;
		}
		else
		{
			this->push_layout(cmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			resolve_surface->change_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		vk::unresolve_image(cmd, this, resolve_surface.get());

		if (!is_depth_surface()) [[likely]]
		{
			vk::insert_image_memory_barrier(
				cmd, this->value,
				this->current_layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
				range);

			vk::insert_image_memory_barrier(
				cmd, resolve_surface->value,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				range);

			this->current_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			resolve_surface->current_layout = VK_IMAGE_LAYOUT_GENERAL;
		}
		else
		{
			this->pop_layout(cmd);
			resolve_surface->change_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
		}

		msaa_flags &= ~(rsx::surface_state_flags::require_unresolve);
	}

	// Default-initialize memory without loading
	void render_target::clear_memory(vk::command_buffer& cmd, vk::image* surface)
	{
		const auto optimal_layout = (surface->current_layout == VK_IMAGE_LAYOUT_GENERAL) ?
			VK_IMAGE_LAYOUT_GENERAL :
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		surface->push_layout(cmd, optimal_layout);

		VkImageSubresourceRange range{ surface->aspect(), 0, 1, 0, 1 };
		if (surface->aspect() & VK_IMAGE_ASPECT_COLOR_BIT)
		{
			VkClearColorValue color = { {0.f, 0.f, 0.f, 1.f} };
			vkCmdClearColorImage(cmd, surface->value, surface->current_layout, &color, 1, &range);
		}
		else
		{
			VkClearDepthStencilValue clear{ 1.f, 255 };
			vkCmdClearDepthStencilImage(cmd, surface->value, surface->current_layout, &clear, 1, &range);
		}

		surface->pop_layout(cmd);

		if (surface == this)
		{
			state_flags &= ~rsx::surface_state_flags::erase_bkgnd;
		}
	}

	// Load memory from cell and use to initialize the surface
	void render_target::load_memory(vk::command_buffer& cmd)
	{
		auto& upload_heap = *vk::get_upload_heap();
		const bool is_swizzled = (raster_type == rsx::surface_raster_type::swizzle);

		rsx::subresource_layout subres{};
		subres.width_in_block = subres.width_in_texel = surface_width * samples_x;
		subres.height_in_block = subres.height_in_texel = surface_height * samples_y;
		subres.pitch_in_block = rsx_pitch / get_bpp();
		subres.depth = 1;
		subres.data = { vm::get_super_ptr<const std::byte>(base_addr), static_cast<std::span<const std::byte>::size_type>(rsx_pitch * surface_height * samples_y) };

		if (g_cfg.video.resolution_scale_percent == 100 && spp == 1) [[likely]]
		{
			push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			vk::upload_image(cmd, this, { subres }, get_gcm_format(), is_swizzled, 1, aspect(), upload_heap, rsx_pitch, upload_contents_inline);
			pop_layout(cmd);
		}
		else
		{
			vk::image* content = nullptr;
			vk::image* final_dst = (samples() > 1) ? get_resolve_target_safe(cmd) : this;

			// Prepare dst image
			final_dst->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			if (final_dst->width() == subres.width_in_block && final_dst->height() == subres.height_in_block)
			{
				// Possible if MSAA is enabled with 100% resolution scale or
				// surface dimensions are less than resolution scale threshold and no MSAA.
				// Writethrough.
				content = final_dst;
			}
			else
			{
				content = vk::get_typeless_helper(format(), format_class(), subres.width_in_block, subres.height_in_block);
				content->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			}

			// Load Cell data into temp buffer
			vk::upload_image(cmd, content, { subres }, get_gcm_format(), is_swizzled, 1, aspect(), upload_heap, rsx_pitch, upload_contents_inline);

			// Write into final image
			if (content != final_dst)
			{
				// Avoid layout push/pop on scratch memory by setting explicit layout here
				content->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

				vk::copy_scaled_image(cmd, content, final_dst,
					{ 0, 0, subres.width_in_block, subres.height_in_block },
					{ 0, 0, static_cast<s32>(final_dst->width()), static_cast<s32>(final_dst->height()) },
					1, true, aspect() == VK_IMAGE_ASPECT_COLOR_BIT ? VK_FILTER_LINEAR : VK_FILTER_NEAREST);
			}

			final_dst->pop_layout(cmd);

			if (samples() > 1)
			{
				// Trigger unresolve
				msaa_flags = rsx::surface_state_flags::require_unresolve;
			}
		}

		state_flags &= ~rsx::surface_state_flags::erase_bkgnd;
	}

	void render_target::initialize_memory(vk::command_buffer& cmd, rsx::surface_access access)
	{
		const bool is_depth = is_depth_surface();
		const bool should_read_buffers = is_depth ? !!g_cfg.video.read_depth_buffer : !!g_cfg.video.read_color_buffers;

		if (!should_read_buffers)
		{
			clear_memory(cmd, this);

			if (samples() > 1 && access.is_transfer_or_read())
			{
				// Only clear the resolve surface if reading from it, otherwise it's a waste
				clear_memory(cmd, get_resolve_target_safe(cmd));
			}

			msaa_flags = rsx::surface_state_flags::ready;
		}
		else
		{
			load_memory(cmd);
		}
	}

	vk::viewable_image* render_target::get_surface(rsx::surface_access access_type)
	{
		if (samples() == 1 || access_type == rsx::surface_access::shader_write)
		{
			return this;
		}

		// A read barrier should have been called before this!
		ensure(resolve_surface); // "Read access without explicit barrier"
		ensure(!(msaa_flags & rsx::surface_state_flags::require_resolve));
		return resolve_surface.get();
	}

	bool render_target::is_depth_surface() const
	{
		return !!(aspect() & VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	void render_target::release_ref(vk::viewable_image* t) const
	{
		static_cast<vk::render_target*>(t)->release();
	}

	bool render_target::matches_dimensions(u16 _width, u16 _height) const
	{
		// Use forward scaling to account for rounding and clamping errors
		const auto [scaled_w, scaled_h] = rsx::apply_resolution_scale<true>(_width, _height);
		return (scaled_w == width()) && (scaled_h == height());
	}

	void render_target::texture_barrier(vk::command_buffer& cmd)
	{
		if (samples() == 1)
		{
			if (!write_barrier_sync_tag) write_barrier_sync_tag++; // Activate barrier sync
			cyclic_reference_sync_tag = write_barrier_sync_tag;    // Match tags
		}

		vk::insert_texture_barrier(cmd, this, VK_IMAGE_LAYOUT_GENERAL);
	}

	void render_target::reset_surface_counters()
	{
		frame_tag = 0;
		write_barrier_sync_tag = 0;
	}

	image_view* render_target::get_view(u32 remap_encoding, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap, VkImageAspectFlags mask)
	{
		if (remap_encoding == VK_REMAP_VIEW_MULTISAMPLED)
		{
			// Special remap flag, intercept here
			return vk::viewable_image::get_view(VK_REMAP_IDENTITY, remap, mask);
		}

		if (!resolve_surface) [[likely]]
		{
			return vk::viewable_image::get_view(remap_encoding, remap, mask);
		}
		else
		{
			return resolve_surface->get_view(remap_encoding, remap, mask);
		}
	}

	void render_target::memory_barrier(vk::command_buffer& cmd, rsx::surface_access access)
	{
		const bool is_depth = is_depth_surface();
		const bool should_read_buffers = is_depth ? !!g_cfg.video.read_depth_buffer : !!g_cfg.video.read_color_buffers;

		if (should_read_buffers)
		{
			// TODO: Decide what to do when memory loads are disabled but the underlying has memory changed
			// NOTE: Assume test() is expensive when in a pinch
			if (last_use_tag && state_flags == rsx::surface_state_flags::ready && !test())
			{
				// TODO: Figure out why merely returning and failing the test does not work when reading (TLoU)
				// The result should have been the same either way
				state_flags |= rsx::surface_state_flags::erase_bkgnd;
			}
		}

		if (access == rsx::surface_access::shader_write && write_barrier_sync_tag != 0)
		{
			if (current_layout == VK_IMAGE_LAYOUT_GENERAL)
			{
				if (write_barrier_sync_tag != cyclic_reference_sync_tag)
				{
					// This barrier catches a very specific case where 2 draw calls are executed with general layout (cyclic ref) but no texture barrier in between.
					// This happens when a cyclic ref is broken. In this case previous draw must finish drawing before the new one renders to avoid current draw breaking previous one.
					VkPipelineStageFlags src_stage, dst_stage;
					VkAccessFlags src_access, dst_access;

					if (!is_depth_surface()) [[likely]]
					{
						src_stage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
						dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
						src_access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
						dst_access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					}
					else
					{
						src_stage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
						dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
						src_access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
						dst_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					}

					vk::insert_image_memory_barrier(cmd, value, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
						src_stage, dst_stage, src_access, dst_access, { aspect(), 0, 1, 0, 1 });

					write_barrier_sync_tag = 0; // Disable for next draw
				}
				else
				{
					// Synced externally for this draw
					write_barrier_sync_tag++;
				}
			}
			else
			{
				write_barrier_sync_tag = 0; // Disable
			}
		}

		if (old_contents.empty()) [[likely]]
		{
			if (state_flags & rsx::surface_state_flags::erase_bkgnd)
			{
				// NOTE: This step CAN introduce MSAA flags!
				initialize_memory(cmd, access);

				ensure(state_flags == rsx::surface_state_flags::ready);
				on_write(rsx::get_shared_tag(), static_cast<rsx::surface_state_flags>(msaa_flags));
			}

			if (msaa_flags & rsx::surface_state_flags::require_resolve)
			{
				if (access.is_transfer_or_read())
				{
					// Only do this step when read access is required
					get_resolve_target_safe(cmd);
					resolve(cmd);
				}
			}
			else if (msaa_flags & rsx::surface_state_flags::require_unresolve)
			{
				if (access == rsx::surface_access::shader_write)
				{
					// Only do this step when it is needed to start rendering
					ensure(resolve_surface);
					unresolve(cmd);
				}
			}

			return;
		}

			// Memory transfers
		vk::image* target_image = (samples() > 1) ? get_resolve_target_safe(cmd) : this;
		vk::blitter hw_blitter;
		const auto dst_bpp = get_bpp();

		unsigned first = prepare_rw_barrier_for_transfer(this);
		const bool accept_all = (last_use_tag && test());
		bool optimize_copy = true;
		u64  newest_tag = 0;

		for (auto i = first; i < old_contents.size(); ++i)
		{
			auto& section = old_contents[i];
			auto src_texture = static_cast<vk::render_target*>(section.source);
			src_texture->read_barrier(cmd);

			if (!accept_all && !src_texture->test()) [[likely]]
			{
				// If this surface is intact, accept all incoming data as it is guaranteed to be safe
				// If this surface has not been initialized or is dirty, do not add more dirty data to it
				continue;
			}

			const auto src_bpp = src_texture->get_bpp();
			rsx::typeless_xfer typeless_info{};

			if (src_texture->aspect() != aspect() ||
				!formats_are_bitcast_compatible(this, src_texture))
			{
				typeless_info.src_is_typeless = true;
				typeless_info.src_context = rsx::texture_upload_context::framebuffer_storage;
				typeless_info.src_native_format_override = static_cast<u32>(info.format);
				typeless_info.src_gcm_format = src_texture->get_gcm_format();
				typeless_info.src_scaling_hint = f32(src_bpp) / dst_bpp;
			}

			section.init_transfer(this);
			auto src_area = section.src_rect();
			auto dst_area = section.dst_rect();

			if (g_cfg.video.antialiasing_level != msaa_level::none)
			{
				src_texture->transform_pixels_to_samples(src_area);
				this->transform_pixels_to_samples(dst_area);
			}

			bool memory_load = true;
			if (dst_area.x1 == 0 && dst_area.y1 == 0 &&
				unsigned(dst_area.x2) == target_image->width() && unsigned(dst_area.y2) == target_image->height())
			{
				// Skip a bunch of useless work
				state_flags &= ~(rsx::surface_state_flags::erase_bkgnd);
				msaa_flags = rsx::surface_state_flags::ready;

				memory_load = false;
				stencil_init_flags = src_texture->stencil_init_flags;
			}
			else if (state_flags & rsx::surface_state_flags::erase_bkgnd)
			{
				// Might introduce MSAA flags
				initialize_memory(cmd, rsx::surface_access::memory_write);
				ensure(state_flags == rsx::surface_state_flags::ready);
			}

			if (msaa_flags & rsx::surface_state_flags::require_resolve)
			{
				// Need to forward resolve this
				resolve(cmd);
			}

			if (samples() > 1)
			{
				// Ensure a writable surface exists for this surface
				get_resolve_target_safe(cmd);
			}

			if (src_texture->samples() > 1)
			{
				// Ensure a readable surface exists for the source
				src_texture->get_resolve_target_safe(cmd);
			}

			hw_blitter.scale_image(
				cmd,
				src_texture->get_surface(rsx::surface_access::transfer_read),
				this->get_surface(rsx::surface_access::transfer_write),
				src_area,
				dst_area,
				/*linear?*/false, typeless_info);

			optimize_copy = optimize_copy && !memory_load;
			newest_tag = src_texture->last_use_tag;
		}

		if (!newest_tag) [[unlikely]]
		{
			// Underlying memory has been modified and we could not find valid data to fill it
			clear_rw_barrier();

			state_flags |= rsx::surface_state_flags::erase_bkgnd;
			initialize_memory(cmd, access);
			ensure(state_flags == rsx::surface_state_flags::ready);
		}

		// NOTE: Optimize flag relates to stencil resolve/unresolve for NVIDIA.
		on_write_copy(newest_tag, optimize_copy);

		if (access == rsx::surface_access::shader_write && samples() > 1)
		{
			// Write barrier, must initialize
			unresolve(cmd);
		}
	}
}
