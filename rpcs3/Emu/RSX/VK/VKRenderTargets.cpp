#include "vkutils/data_heap.h"
#include "VKRenderTargets.h"
#include "VKResourceManager.h"
#include "Emu/RSX/rsx_methods.h"
#include "Emu/RSX/RSXThread.h"

#include "Emu/RSX/Common/tiled_dma_copy.hpp"

namespace vk
{
	namespace surface_cache_utils
	{
		void dispose(vk::buffer* buf)
		{
			auto obj = vk::disposable_t::make(buf);
			vk::get_resource_manager()->dispose(obj);
		}
	}

	void surface_cache::destroy()
	{
		invalidate_all();
		invalidated_resources.clear();
	}

	u64 surface_cache::get_surface_cache_memory_quota(u64 total_device_memory)
	{
		total_device_memory /= 0x100000;
		u64 quota = 0;

		if (total_device_memory >= 2048)
		{
			quota = std::min<u64>(6144, (total_device_memory * 40) / 100);
		}
		else if (total_device_memory >= 1024)
		{
			quota = std::max<u64>(512, (total_device_memory * 30) / 100);
		}
		else if (total_device_memory >= 768)
		{
			quota = 256;
		}
		else
		{
			// Remove upto 128MB but at least aim for half of available VRAM
			quota = std::min<u64>(128, total_device_memory / 2);
		}

		return quota * 0x100000;
	}

	bool surface_cache::can_collapse_surface(const std::unique_ptr<vk::render_target>& surface, rsx::problem_severity severity)
	{
		if (severity < rsx::problem_severity::fatal &&
			vk::vmm_determine_memory_load_severity() < rsx::problem_severity::fatal)
		{
			// We may be able to allocate what we need.
			return true;
		}

		// Check if we need to do any allocations. Do not collapse in such a situation otherwise
		if (surface->samples() > 1 && !surface->resolve_surface)
		{
			return false;
		}

		// Resolve target does exist. Scan through the entire collapse chain
		for (auto& region : surface->old_contents)
		{
			// FIXME: This is just lazy
			auto proxy = std::unique_ptr<vk::render_target>(vk::as_rtt(region.source));
			const bool collapsible = can_collapse_surface(proxy, severity);
			proxy.release();

			if (!collapsible)
			{
				return false;
			}
		}

		return true;
	}

	bool surface_cache::handle_memory_pressure(vk::command_buffer& cmd, rsx::problem_severity severity)
	{
		bool any_released = rsx::surface_store<surface_cache_traits>::handle_memory_pressure(cmd, severity);

		if (severity >= rsx::problem_severity::fatal)
		{
			std::vector<std::unique_ptr<vk::viewable_image>> resolve_target_cache;
			std::vector<vk::render_target*> deferred_spills;
			auto gc = vk::get_resource_manager();

			// Drop MSAA resolve/unresolve caches. Only trigger when a hard sync is guaranteed to follow else it will cause even more problems!
			// 2-pass to ensure resources are available where they are most needed
			auto relieve_memory_pressure = [&](auto& list, const utils::address_range32& range)
			{
				for (auto it = list.begin_range(range); it != list.end(); ++it)
				{
					auto& rtt = it->second;
					if (!rtt->spill_request_tag || rtt->spill_request_tag < rtt->last_rw_access_tag)
					{
						// We're not going to be spilling into system RAM. If a MSAA resolve target exists, remove it to save memory.
						if (rtt->resolve_surface)
						{
							resolve_target_cache.emplace_back(std::move(rtt->resolve_surface));
							rtt->msaa_flags |= rsx::surface_state_flags::require_resolve;
							any_released |= true;
						}

						rtt->spill_request_tag = 0;
						continue;
					}

					if (rtt->resolve_surface || rtt->samples() == 1)
					{
						// Can spill immediately. Do it.
						ensure(rtt->spill(cmd, resolve_target_cache));
						any_released |= true;
						continue;
					}

					deferred_spills.push_back(rtt.get());
				}
			};

			// 1. Spill an strip any 'invalidated resources'. At this point it doesn't matter and we donate to the resolve cache which is a plus.
			for (auto& surface : invalidated_resources)
			{
				if (!surface->value && !surface->resolve_surface)
				{
					// Unspilled resources can have no value but have a resolve surface used for read
					continue;
				}

				// Only spill anything with references. Other surfaces already marked for removal should be inevitably deleted when it is time to free_invalidated
				if (surface->has_refs() && (surface->resolve_surface || surface->samples() == 1))
				{
					ensure(surface->spill(cmd, resolve_target_cache));
					any_released |= true;
				}
				else if (surface->resolve_surface)
				{
					ensure(!surface->has_refs());
					resolve_target_cache.emplace_back(std::move(surface->resolve_surface));
					surface->msaa_flags |= rsx::surface_state_flags::require_resolve;
					any_released |= true;
				}
				else if (surface->has_refs())
				{
					deferred_spills.push_back(surface.get());
				}
			}

			// 2. Scan the list and spill resources that can be spilled immediately if requested. Also gather resources from those that don't need it.
			relieve_memory_pressure(m_render_targets_storage, m_render_targets_memory_range);
			relieve_memory_pressure(m_depth_stencil_storage, m_depth_stencil_memory_range);

			// 3. Write to system heap everything marked to spill
			for (auto& surface : deferred_spills)
			{
				any_released |= surface->spill(cmd, resolve_target_cache);
			}

			// 4. Cleanup; removes all the resources used up here that are no longer needed for the moment
			for (auto& data : resolve_target_cache)
			{
				gc->dispose(data);
			}
		}

		return any_released;
	}

	void surface_cache::trim(vk::command_buffer& cmd, rsx::problem_severity memory_pressure)
	{
		run_cleanup_internal(cmd, rsx::problem_severity::moderate, 300, [](vk::command_buffer& cmd)
		{
			if (!cmd.is_recording())
			{
				cmd.begin();
			}
		});

		const u64 last_finished_frame = vk::get_last_completed_frame_id();
		invalidated_resources.remove_if([&](std::unique_ptr<vk::render_target>& rtt)
		{
			ensure(rtt->frame_tag != 0);

			if (rtt->has_refs())
			{
				// Actively in use, likely for a reading pass.
				// Call handle_memory_pressure before calling this method.
				return false;
			}

			if (rtt->frame_tag >= last_finished_frame)
			{
				// RTT itself still in use by the frame.
				return false;
			}

			if (!rtt->old_contents.empty())
			{
				rtt->clear_rw_barrier();
			}

			if (rtt->resolve_surface && memory_pressure >= rsx::problem_severity::moderate)
			{
				// We do not need to keep resolve targets around.
				// TODO: We should surrender this to an image cache immediately for reuse.
				vk::get_resource_manager()->dispose(rtt->resolve_surface);
			}

			switch (memory_pressure)
			{
			case rsx::problem_severity::low:
				return (rtt->unused_check_count() >= 2);
			case rsx::problem_severity::moderate:
				return (rtt->unused_check_count() >= 1);
			case rsx::problem_severity::severe:
			case rsx::problem_severity::fatal:
				// We're almost dead anyway. Remove forcefully.
				vk::get_resource_manager()->dispose(rtt);
				return true;
			default:
				fmt::throw_exception("Unreachable");
			}
		});
	}

	bool surface_cache::is_overallocated()
	{
		const auto surface_cache_vram_load = vmm_get_application_pool_usage(VMM_ALLOCATION_POOL_SURFACE_CACHE);
		const auto surface_cache_allocation_quota = get_surface_cache_memory_quota(get_current_renderer()->get_memory_mapping().device_local_total_bytes);
		return (surface_cache_vram_load > surface_cache_allocation_quota);
	}

	bool surface_cache::spill_unused_memory()
	{
		// Determine how much memory we need to save to system RAM if any
		const u64 current_surface_cache_memory = vk::vmm_get_application_pool_usage(VMM_ALLOCATION_POOL_SURFACE_CACHE);
		const u64 total_device_memory = vk::get_current_renderer()->get_memory_mapping().device_local_total_bytes;
		const u64 target_memory = get_surface_cache_memory_quota(total_device_memory);

		rsx_log.warning("Surface cache memory usage is %lluM", current_surface_cache_memory / 0x100000);
		if (current_surface_cache_memory < target_memory)
		{
			rsx_log.warning("Surface cache memory usage is very low. Will not spill contents to RAM");
			return false;
		}

		// Very slow, but should only be called when the situation is dire
		std::vector<render_target*> sorted_list;
		sorted_list.reserve(1024);

		auto process_list_function = [&](auto& list, const utils::address_range32& range)
		{
			for (auto it = list.begin_range(range); it != list.end(); ++it)
			{
				// NOTE: Check if memory is available instead of value in case we ran out of memory during unspill
				auto& surface = it->second;
				if (surface->memory && !surface->is_bound)
				{
					sorted_list.push_back(surface.get());
				}
			}
		};

		process_list_function(m_render_targets_storage, m_render_targets_memory_range);
		process_list_function(m_depth_stencil_storage, m_depth_stencil_memory_range);

		std::sort(sorted_list.begin(), sorted_list.end(), FN(x->last_rw_access_tag < y->last_rw_access_tag));

		// Remove upto target_memory bytes from VRAM
		u64 bytes_spilled = 0;
		const u64 bytes_to_remove = current_surface_cache_memory - target_memory;
		const u64 spill_time = rsx::get_shared_tag();

		for (auto& surface : sorted_list)
		{
			bytes_spilled += surface->memory->size();
			surface->spill_request_tag = spill_time;

			if (bytes_spilled >= bytes_to_remove)
			{
				break;
			}
		}

		rsx_log.warning("Surface cache will attempt to spill %llu bytes.", bytes_spilled);
		return (bytes_spilled > 0);
	}

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
				VMM_ALLOCATION_POOL_SURFACE_CACHE,
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

	std::vector<VkBufferImageCopy> render_target::build_spill_transfer_descriptors(vk::image* target)
	{
		std::vector<VkBufferImageCopy> result;
		result.reserve(2);

		result.push_back({});
		auto& rgn = result.back();
		rgn.imageExtent.width = target->width();
		rgn.imageExtent.height = target->height();
		rgn.imageExtent.depth = 1;
		rgn.imageSubresource.aspectMask = target->aspect();
		rgn.imageSubresource.layerCount = 1;

		if (aspect() == (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
		{
			result.push_back(rgn);
			rgn.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			result.back().imageSubresource.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
			result.back().bufferOffset = target->width() * target->height() * 4;
		}

		return result;
	}

	bool render_target::spill(vk::command_buffer& cmd, std::vector<std::unique_ptr<vk::viewable_image>>& resolve_cache)
	{
		u64 element_size;
		switch (const auto fmt = format())
		{
		case VK_FORMAT_D32_SFLOAT:
			element_size = 4;
			break;
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
			element_size = 5;
			break;
		default:
			element_size = get_format_texel_width(fmt);
			break;
		}

		vk::viewable_image* src = nullptr;
		if (samples() == 1) [[likely]]
		{
			ensure(value);
			src = this;
		}
		else if (resolve_surface)
		{
			src = resolve_surface.get();
		}
		else
		{
			const auto transfer_w = width() * samples_x;
			const auto transfer_h = height() * samples_y;

			for (auto& surface : resolve_cache)
			{
				if (surface->format() == format() &&
					surface->width() == transfer_w &&
					surface->height() == transfer_h)
				{
					src = surface.get();
					break;
				}
			}

			if (!src)
			{
				if (vmm_determine_memory_load_severity() <= rsx::problem_severity::moderate)
				{
					// We have some freedom to allocate something. Add to the shared cache
					src = get_resolve_target_safe(cmd);
				}
				else
				{
					// TODO: Spill to DMA buf
					// For now, just skip this one if we don't have the capacity for it
					rsx_log.warning("Could not spill memory due to resolve failure. Will ignore spilling for the moment.");
					return false;
				}
			}

			msaa_flags |= rsx::surface_state_flags::require_resolve;
		}

		// If a resolve is requested, move data to the target
		if (msaa_flags & rsx::surface_state_flags::require_resolve)
		{
			ensure(samples() > 1);
			const bool borrowed = [&]()
			{
				if (src != resolve_surface.get())
				{
					ensure(!resolve_surface);
					resolve_surface.reset(src);
					return true;
				}

				return false;
			}();

			resolve(cmd);

			if (borrowed)
			{
				resolve_surface.release();
			}
		}

		const auto pdev = vk::get_current_renderer();
		const auto alloc_size = element_size * src->width() * src->height();

		m_spilled_mem = std::make_unique<vk::buffer>(*pdev, alloc_size, pdev->get_memory_mapping().host_visible_coherent,
			0, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 0, VMM_ALLOCATION_POOL_UNDEFINED);

		const auto regions = build_spill_transfer_descriptors(src);
		src->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		vkCmdCopyImageToBuffer(cmd, src->value, src->current_layout, m_spilled_mem->value, ::size32(regions), regions.data());

		// Destroy this object through a cloned object
		auto obj = std::unique_ptr<viewable_image>(clone());
		vk::get_resource_manager()->dispose(obj);

		if (resolve_surface)
		{
			// Just add to the resolve cache and move on
			resolve_cache.emplace_back(std::move(resolve_surface));
		}

		ensure(!memory && !value && views.empty() && !resolve_surface);
		spill_request_tag = 0ull;
		return true;
	}

	void render_target::unspill(vk::command_buffer& cmd)
	{
		// Recreate the image
		const auto pdev = vk::get_current_renderer();
		create_impl(*pdev, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pdev->get_memory_mapping().device_local, VMM_ALLOCATION_POOL_SURFACE_CACHE);
		change_layout(cmd, is_depth_surface() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// Load image from host-visible buffer
		ensure(m_spilled_mem);

		// Data transfer can be skipped if an erase command is being served
		if (!(state_flags & rsx::surface_state_flags::erase_bkgnd))
		{
			// Warn. Ideally this should never happen if you have enough resources
			rsx_log.warning("[PERFORMANCE WARNING] Loading spilled memory back to the GPU. You may want to lower your resolution scaling.");

			vk::image* dst = (samples() > 1) ? get_resolve_target_safe(cmd) : this;
			const auto regions = build_spill_transfer_descriptors(dst);

			dst->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			vkCmdCopyBufferToImage(cmd, m_spilled_mem->value, dst->value, dst->current_layout, ::size32(regions), regions.data());

			if (samples() > 1)
			{
				msaa_flags &= ~rsx::surface_state_flags::require_resolve;
				msaa_flags |= rsx::surface_state_flags::require_unresolve;
			}
		}

		// Delete host-visible buffer
		vk::get_resource_manager()->dispose(m_spilled_mem);
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

		const auto range = get_memory_range();
		rsx::flags32_t upload_flags = upload_contents_inline;
		u32 heap_align = rsx_pitch;

#if DEBUG_DMA_TILING
		std::vector<u8> ext_data;
#endif

		if (auto tiled_region = rsx::get_current_renderer()->get_tiled_memory_region(range))
		{
#if DEBUG_DMA_TILING
			auto real_data = vm::get_super_ptr<u8>(range.start);
			ext_data.resize(tiled_region.tile->size);
			auto detile_func = get_bpp() == 4
				? rsx::detile_texel_data32
				: rsx::detile_texel_data16;

			detile_func(
				ext_data.data(),
				real_data,
				tiled_region.base_address,
				range.start - tiled_region.base_address,
				tiled_region.tile->size,
				tiled_region.tile->bank,
				tiled_region.tile->pitch,
				subres.width_in_block,
				subres.height_in_block
			);
			subres.data = std::span(ext_data);
#else
			const auto [scratch_buf, linear_data_scratch_offset] = vk::detile_memory_block(cmd, tiled_region, range, subres.width_in_block, subres.height_in_block, get_bpp());

			// FIXME: !!EVIL!!
			subres.data = { scratch_buf, linear_data_scratch_offset };
			subres.pitch_in_block = subres.width_in_block;
			upload_flags |= source_is_gpu_resident;
			heap_align = subres.width_in_block * get_bpp();
#endif
		}

		if (g_cfg.video.resolution_scale_percent == 100 && spp == 1) [[likely]]
		{
			push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			vk::upload_image(cmd, this, { subres }, get_gcm_format(), is_swizzled, 1, aspect(), upload_heap, heap_align, upload_flags);
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
				if (content->current_layout == VK_IMAGE_LAYOUT_UNDEFINED)
				{
					content->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
				}
				content->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			}

			// Load Cell data into temp buffer
			vk::upload_image(cmd, content, { subres }, get_gcm_format(), is_swizzled, 1, aspect(), upload_heap, heap_align, upload_flags);

			// Write into final image
			if (content != final_dst)
			{
				// Avoid layout push/pop on scratch memory by setting explicit layout here
				content->pop_layout(cmd);
				content->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

				vk::copy_scaled_image(cmd, content, final_dst,
					{ 0, 0, subres.width_in_block, subres.height_in_block },
					{ 0, 0, static_cast<s32>(final_dst->width()), static_cast<s32>(final_dst->height()) },
					1, true, aspect() == VK_IMAGE_ASPECT_COLOR_BIT ? VK_FILTER_LINEAR : VK_FILTER_NEAREST);

				content->pop_layout(cmd);
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
		last_rw_access_tag = rsx::get_shared_tag();

		if (samples() == 1 || !access_type.is_transfer())
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

	bool render_target::matches_dimensions(u16 _width, u16 _height) const
	{
		// Use forward scaling to account for rounding and clamping errors
		const auto [scaled_w, scaled_h] = rsx::apply_resolution_scale<true>(_width, _height);
		return (scaled_w == width()) && (scaled_h == height());
	}

	void render_target::texture_barrier(vk::command_buffer& cmd)
	{
		const auto is_framebuffer_read_only = is_depth_surface() && !rsx::method_registers.depth_write_enabled();
		const auto supports_fbo_loops = cmd.get_command_pool().get_owner().get_framebuffer_loops_support();
		const auto optimal_layout = supports_fbo_loops ? VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT
			: VK_IMAGE_LAYOUT_GENERAL;

		if (m_cyclic_ref_tracker.can_skip() && current_layout == optimal_layout && is_framebuffer_read_only)
		{
			// If we have back-to-back depth-read barriers, skip subsequent ones
			// If an actual write is happening, this flag will be automatically reset
			return;
		}

		vk::insert_texture_barrier(cmd, this, optimal_layout);
		m_cyclic_ref_tracker.on_insert_texture_barrier();

		if (is_framebuffer_read_only)
		{
			m_cyclic_ref_tracker.allow_skip();
		}
	}

	void render_target::post_texture_barrier(vk::command_buffer& cmd)
	{
		// This is a fall-out barrier after a cyclic ref when the same surface is still bound.
		// In this case, we're just checking that the previous read completes before the next write.
		const bool is_framebuffer_read_only = is_depth_surface() && !rsx::method_registers.depth_write_enabled();
		if (m_cyclic_ref_tracker.can_skip() && is_framebuffer_read_only)
		{
			// Barrier ellided if triggered by a chain of cyclic references with no actual writes
			m_cyclic_ref_tracker.reset();
			return;
		}

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

		vk::insert_image_memory_barrier(cmd, value, current_layout, current_layout,
			src_stage, dst_stage, src_access, dst_access, { aspect(), 0, 1, 0, 1 });

		m_cyclic_ref_tracker.reset();
	}

	void render_target::reset_surface_counters()
	{
		frame_tag = 0;
		m_cyclic_ref_tracker.reset();
	}

	image_view* render_target::get_view(const rsx::texture_channel_remap_t& remap, VkImageAspectFlags mask)
	{
		if (remap.encoded == VK_REMAP_VIEW_MULTISAMPLED)
		{
			// Special remap flag, intercept here
			return vk::viewable_image::get_view(remap.with_encoding(VK_REMAP_IDENTITY), mask);
		}

		return vk::viewable_image::get_view(remap, mask);
	}

	void render_target::memory_barrier(vk::command_buffer& cmd, rsx::surface_access access)
	{
		if (access == rsx::surface_access::gpu_reference)
		{
			// This barrier only requires that an object is made available for GPU usage.
			if (!value)
			{
				unspill(cmd);
			}

			spill_request_tag = 0;
			return;
		}

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

		// Unspill here, because erase flag may have been set above.
		if (!value)
		{
			unspill(cmd);
		}

		if (access == rsx::surface_access::shader_write && m_cyclic_ref_tracker.is_enabled())
		{
			if (current_layout == VK_IMAGE_LAYOUT_GENERAL || current_layout == VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT)
			{
				// Flag draw barrier observed
				m_cyclic_ref_tracker.on_insert_draw_barrier();

				// Check if we've had more draws than barriers so far (fall-out condition)
				if (m_cyclic_ref_tracker.requires_post_loop_barrier())
				{
					post_texture_barrier(cmd);
				}
			}
			else
			{
				// Layouts changed elsewhere. Reset.
				m_cyclic_ref_tracker.reset();
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
				if (access.is_transfer())
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
			src_texture->memory_barrier(cmd, rsx::surface_access::transfer_read);

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
