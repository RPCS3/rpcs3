#include "stdafx.h"
#include "Emu/RSX/VK/VKGSRenderTypes.hpp"
#include "VKTextureCache.h"
#include "VKCompute.h"
#include "VKAsyncScheduler.h"

#include "util/asm.hpp"

namespace vk
{
	u64 hash_image_properties(VkFormat format, u16 w, u16 h, u16 d, u16 mipmaps, VkImageType type, VkImageCreateFlags create_flags, VkSharingMode sharing_mode)
	{
		/**
		* Key layout:
		* 00-08: Format  (Max 255)
		* 08-24: Width   (Max 64K)
		* 24-40: Height  (Max 64K)
		* 40-48: Depth   (Max 255)
		* 48-54: Mipmaps (Max 63)   <- We have some room here, it is not possible to have more than 12 mip levels on PS3 and 16 on PC is pushing it.
		* 54-56: Type    (Max 3)
		* 56-57: Sharing (Max 1)    <- Boolean. Exclusive = 0, shared = 1
		* 57-64: Flags   (Max 127)  <- We have some room here, we only care about a small subset of create flags.
		*/
		ensure(static_cast<u32>(format) < 0xFF);
		return (static_cast<u64>(format) & 0xFF) |
			(static_cast<u64>(w) << 8) |
			(static_cast<u64>(h) << 24) |
			(static_cast<u64>(d) << 40) |
			(static_cast<u64>(mipmaps) << 48) |
			(static_cast<u64>(type) << 54) |
			(static_cast<u64>(sharing_mode) << 56) |
			(static_cast<u64>(create_flags) << 57);
	}

	texture_cache::cached_image_reference_t::cached_image_reference_t(texture_cache* parent, std::unique_ptr<vk::viewable_image>& previous)
	{
		ensure(previous);

		this->parent = parent;
		this->data = std::move(previous);
	}

	texture_cache::cached_image_reference_t::~cached_image_reference_t()
	{
		// Erase layout information to force TOP_OF_PIPE transition next time.
		data->current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		data->current_queue_family = VK_QUEUE_FAMILY_IGNORED;

		// Move this object to the cached image pool
		const auto key = hash_image_properties(data->format(), data->width(), data->height(), data->depth(), data->mipmaps(), data->info.imageType, data->info.flags, data->info.sharingMode);
		std::lock_guard lock(parent->m_cached_pool_lock);

		if (!parent->m_cache_is_exiting)
		{
			parent->m_cached_memory_size += data->memory->size();
			parent->m_cached_images.emplace_front(key, data);
		}
		else
		{
			// Destroy if the cache is closed. The GPU is done with this resource anyway.
			data.reset();
		}
	}

	void cached_texture_section::dma_transfer(vk::command_buffer& cmd, vk::image* src, const areai& src_area, const utils::address_range32& valid_range, u32 pitch)
	{
		ensure(src->samples() == 1);

		if (!m_device)
		{
			m_device = &cmd.get_command_pool().get_owner();
		}

		if (dma_fence)
		{
			// NOTE: This can be reached if previously synchronized, or a special path happens.
			// If a hard flush occurred while this surface was flush_always the cache would have reset its protection afterwards.
			// DMA resource would still be present but already used to flush previously.
			vk::get_resource_manager()->dispose(dma_fence);
		}

		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		src->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		const auto internal_bpp = vk::get_format_texel_width(src->format());
		const auto transfer_width = static_cast<u32>(src_area.width());
		const auto transfer_height = static_cast<u32>(src_area.height());
		real_pitch = internal_bpp * transfer_width;
		rsx_pitch = pitch;

		const bool require_format_conversion = !!(src->aspect() & VK_IMAGE_ASPECT_STENCIL_BIT) || src->format() == VK_FORMAT_D32_SFLOAT;
		const auto tiled_region = rsx::get_current_renderer()->get_tiled_memory_region(valid_range);
		const bool require_tiling = !!tiled_region;
		const bool require_gpu_transform = require_format_conversion || pack_unpack_swap_bytes || require_tiling;

		auto dma_sync_region = valid_range;
		dma_mapping_handle dma_mapping = { 0, nullptr };

		auto dma_sync = [&dma_sync_region, &dma_mapping](bool load, bool force = false)
		{
			if (dma_mapping.second && !force)
			{
				return;
			}

			dma_mapping = vk::map_dma(dma_sync_region.start, dma_sync_region.length());
			if (load)
			{
				vk::load_dma(dma_sync_region.start, dma_sync_region.length());
			}
		};

		if (require_gpu_transform)
		{
			const auto transfer_pitch = real_pitch;
			const auto task_length = transfer_pitch * src_area.height();
			auto working_buffer_length = calculate_working_buffer_size(task_length, src->aspect());

#if !DEBUG_DMA_TILING
			if (require_tiling)
			{
				// Safety padding
				working_buffer_length += tiled_region.tile->size;

				// Calculate actual working section for the memory op
				dma_sync_region = tiled_region.tile_align(dma_sync_region);
			}
#endif

			auto working_buffer = vk::get_scratch_buffer(cmd, working_buffer_length);
			u32 result_offset = 0;

			VkBufferImageCopy region = {};
			region.imageSubresource = { src->aspect(), 0, 0, 1 };
			region.imageOffset = { src_area.x1, src_area.y1, 0 };
			region.imageExtent = { transfer_width, transfer_height, 1 };

			bool require_rw_barrier = true;
			image_readback_options_t xfer_options{};
			xfer_options.swap_bytes = require_format_conversion && pack_unpack_swap_bytes;
			vk::copy_image_to_buffer(cmd, src, working_buffer, region, xfer_options);

			// NOTE: For depth/stencil formats, copying to buffer and byteswap are combined into one step above
			if (pack_unpack_swap_bytes && !require_format_conversion)
			{
				const auto texel_layout = vk::get_format_element_size(src->format());
				const auto elem_size = texel_layout.first;
				vk::cs_shuffle_base* shuffle_kernel;

				if (elem_size == 2)
				{
					shuffle_kernel = vk::get_compute_task<vk::cs_shuffle_16>();
				}
				else if (elem_size == 4)
				{
					shuffle_kernel = vk::get_compute_task<vk::cs_shuffle_32>();
				}
				else
				{
					ensure(get_context() == rsx::texture_upload_context::dma);
					shuffle_kernel = nullptr;
				}

				if (shuffle_kernel)
				{
					vk::insert_buffer_memory_barrier(cmd, working_buffer->value, 0, task_length,
						VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

					shuffle_kernel->run(cmd, working_buffer, task_length);

					if (!require_tiling)
					{
						vk::insert_buffer_memory_barrier(cmd, working_buffer->value, 0, task_length,
							VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
							VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

						require_rw_barrier = false;
					}
				}
			}

			if (require_tiling)
			{
#if !DEBUG_DMA_TILING
				// Compute -> Compute barrier
				vk::insert_buffer_memory_barrier(cmd, working_buffer->value, 0, task_length,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

				// We don't need to calibrate write if two conditions are met:
				// 1. The start offset of our 2D region is a multiple of 64 lines
				// 2. We use the whole pitch.
				// If these conditions are not met, we need to upload the entire tile (or at least the affected tiles wholly)

				// FIXME: There is a 3rd condition - write onto already-persisted range. e.g One transfer copies half the image then the other half is copied later.
				// We don't need to load again for the second copy in that scenario.

				if (valid_range.start != dma_sync_region.start || real_pitch != tiled_region.tile->pitch)
				{
					// Tile indices run to the end of the row (full pitch).
					// Tiles address outside their 64x64 area too, so we need to actually load the whole thing and "fill in" missing blocks.
					// Visualizing "hot" pixels when doing a partial copy is very revealing, there's lots of data from the padding areas to be filled in.

					dma_sync(true);
					ensure(dma_mapping.second);

					// Upload memory to the working buffer
					const auto dst_offset = task_length; // Append to the end of the input
					VkBufferCopy mem_load{};
					mem_load.srcOffset = dma_mapping.first;
					mem_load.dstOffset = dst_offset;
					mem_load.size = dma_sync_region.length();
					vkCmdCopyBuffer(cmd, dma_mapping.second->value, working_buffer->value, 1, &mem_load);

					// Transfer -> Compute barrier
					vk::insert_buffer_memory_barrier(cmd, working_buffer->value, dst_offset, dma_sync_region.length(),
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT);
				}

				// Prepare payload
				const RSX_detiler_config config =
				{
					.tile_base_address = tiled_region.base_address,
					.tile_base_offset = valid_range.start - tiled_region.base_address,
					.tile_rw_offset = dma_sync_region.start - tiled_region.base_address,
					.tile_size = tiled_region.tile->size,
					.tile_pitch = tiled_region.tile->pitch,
					.bank = tiled_region.tile->bank,

					.dst = working_buffer,
					.dst_offset = task_length,
					.src = working_buffer,
					.src_offset = 0,

					// TODO: Check interaction with anti-aliasing
					.image_width = static_cast<u16>(transfer_width),
					.image_height = static_cast<u16>(transfer_height),
					.image_pitch = real_pitch,
					.image_bpp = context == rsx::texture_upload_context::dma ? internal_bpp : rsx::get_format_block_size_in_bytes(gcm_format)
				};

				// Execute
				const auto job = vk::get_compute_task<vk::cs_tile_memcpy<RSX_detiler_op::encode>>();
				job->run(cmd, config);

				// Update internal variables
				result_offset = task_length;
				real_pitch = tiled_region.tile->pitch; // We're always copying the full image. In case of partials we're "filling in" blocks, not doing partial 2D copies.
				require_rw_barrier = true;

#if VISUALIZE_GPU_TILING
				if (g_cfg.video.renderdoc_compatiblity)
				{
					vk::insert_buffer_memory_barrier(cmd, working_buffer->value, result_offset, working_buffer_length,
						VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

					// Debug write
					auto scratch_img = vk::get_typeless_helper(VK_FORMAT_B8G8R8A8_UNORM, RSX_FORMAT_CLASS_COLOR, tiled_region.tile->pitch / 4, 768);
					scratch_img->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

					VkBufferImageCopy dbg_copy{};
					dbg_copy.bufferOffset = config.dst_offset;
					dbg_copy.imageExtent.width = width;
					dbg_copy.imageExtent.height = height;
					dbg_copy.imageExtent.depth = 1;
					dbg_copy.bufferRowLength = tiled_region.tile->pitch / 4;
					dbg_copy.imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 };
					vk::copy_buffer_to_image(cmd, working_buffer, scratch_img, dbg_copy);

					scratch_img->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
				}
#endif

#endif
			}

			if (require_rw_barrier)
			{
				vk::insert_buffer_memory_barrier(cmd, working_buffer->value, result_offset, dma_sync_region.length(),
					VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
			}

			if (rsx_pitch == real_pitch) [[likely]]
			{
				dma_sync(false);

				VkBufferCopy copy = {};
				copy.srcOffset = result_offset;
				copy.dstOffset = dma_mapping.first;
				copy.size = dma_sync_region.length();
				vkCmdCopyBuffer(cmd, working_buffer->value, dma_mapping.second->value, 1, &copy);
			}
			else
			{
				dma_sync(true);

				std::vector<VkBufferCopy> copy;
				copy.reserve(transfer_height);

				u32 dst_offset = dma_mapping.first;
				u32 src_offset = result_offset;

				for (unsigned row = 0; row < transfer_height; ++row)
				{
					copy.push_back({ src_offset, dst_offset, transfer_pitch });
					src_offset += real_pitch;
					dst_offset += rsx_pitch;
				}

				vkCmdCopyBuffer(cmd, working_buffer->value, dma_mapping.second->value, transfer_height, copy.data());
			}
		}
		else
		{
			dma_sync(false);

			VkBufferImageCopy region = {};
			region.bufferRowLength = (rsx_pitch / internal_bpp);
			region.imageSubresource = { src->aspect(), 0, 0, 1 };
			region.imageOffset = { src_area.x1, src_area.y1, 0 };
			region.imageExtent = { transfer_width, transfer_height, 1 };

			region.bufferOffset = dma_mapping.first;
			vkCmdCopyImageToBuffer(cmd, src->value, src->current_layout, dma_mapping.second->value, 1, &region);
		}

		src->pop_layout(cmd);

		VkBufferMemoryBarrier2KHR mem_barrier =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR,
			.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR,      // Finish all transfer...
			.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR,
			.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR,  // ...before proceeding with any command
			.dstAccessMask = 0,
			.buffer = dma_mapping.second->value,
			.offset = dma_mapping.first,
			.size = valid_range.length()
		};

		// Create event object for this transfer and queue signal op
		dma_fence = std::make_unique<vk::event>(*m_device, sync_domain::host);
		dma_fence->signal(cmd,
		{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.bufferMemoryBarrierCount = 1,
			.pBufferMemoryBarriers = &mem_barrier
		});

		// Set cb flag for queued dma operations
		cmd.set_flag(vk::command_buffer::cb_has_dma_transfer);

		if (get_context() == rsx::texture_upload_context::dma)
		{
			// Save readback hint in case transformation is required later
			switch (internal_bpp)
			{
			case 2:
				gcm_format = CELL_GCM_TEXTURE_R5G6B5;
				break;
			case 4:
			default:
				gcm_format = CELL_GCM_TEXTURE_A8R8G8B8;
				break;
			}
		}

		synchronized = true;
		sync_timestamp = rsx::get_shared_tag();
	}

	void texture_cache::on_section_destroyed(cached_texture_section& tex)
	{
		if (tex.is_managed() && tex.exists())
		{
			auto disposable = vk::disposable_t::make(new cached_image_reference_t(this, tex.get_texture()));
			vk::get_resource_manager()->dispose(disposable);
		}
	}

	void texture_cache::clear()
	{
		{
			std::lock_guard lock(m_cached_pool_lock);
			m_cache_is_exiting = true;
		}
		baseclass::clear();

		m_cached_images.clear();
		m_cached_memory_size = 0;
	}

	void texture_cache::copy_transfer_regions_impl(vk::command_buffer& cmd, vk::image* dst, const std::vector<copy_region_descriptor>& sections_to_transfer) const
	{
		const auto dst_aspect = dst->aspect();
		const auto dst_bpp = vk::get_format_texel_width(dst->format());

		for (const auto& section : sections_to_transfer)
		{
			if (!section.src)
			{
				continue;
			}

			// Generates a region to write data to the final destination
			const auto get_output_region = [&](s32 in_x, s32 in_y, u32 w, u32 h, vk::image* data_src)
			{
				VkImageCopy copy_rgn = {
					.srcSubresource = { data_src->aspect(), 0, 0, 1},
					.srcOffset = { in_x, in_y, 0 },
					.dstSubresource = { dst_aspect, section.level, 0, 1 },
					.dstOffset = { section.dst_x, section.dst_y, 0 },
					.extent = { w, h, 1 }
				};

				if (dst->info.imageType == VK_IMAGE_TYPE_3D)
				{
					copy_rgn.dstOffset.z = section.dst_z;
				}
				else
				{
					copy_rgn.dstSubresource.baseArrayLayer = section.dst_z;
				}

				return copy_rgn;
			};

			const bool typeless = section.src->aspect() != dst_aspect ||
				!formats_are_bitcast_compatible(dst, section.src);

			// Avoid inserting unnecessary barrier GENERAL->TRANSFER_SRC->GENERAL in active render targets
			const auto preferred_layout = (section.src->current_layout != VK_IMAGE_LAYOUT_GENERAL) ?
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;

			section.src->push_layout(cmd, preferred_layout);

			auto src_image = section.src;
			auto src_x = section.src_x;
			auto src_y = section.src_y;
			auto src_w = section.src_w;
			auto src_h = section.src_h;

			rsx::flags32_t transform = section.xform;
			if (section.xform == rsx::surface_transform::coordinate_transform)
			{
				// Dimensions were given in 'dst' space. Work out the real source coordinates
				const auto src_bpp = vk::get_format_texel_width(section.src->format());
				src_x = (src_x * dst_bpp) / src_bpp;
				src_w = utils::aligned_div<u16>(src_w * dst_bpp, src_bpp);

				transform &= ~(rsx::surface_transform::coordinate_transform);
			}

			if (auto surface = dynamic_cast<vk::render_target*>(section.src))
			{
				surface->transform_samples_to_pixels(src_x, src_w, src_y, src_h);
			}

			if (typeless) [[unlikely]]
			{
				const auto src_bpp = vk::get_format_texel_width(section.src->format());
				const u16 convert_w = u16(src_w * src_bpp) / dst_bpp;
				const u16 convert_x = u16(src_x * src_bpp) / dst_bpp;

				if (convert_w == section.dst_w && src_h == section.dst_h &&
					transform == rsx::surface_transform::identity &&
					section.level == 0 && section.dst_z == 0)
				{
					// Optimization to avoid double transfer
					// TODO: Handle level and layer offsets
					const areai src_rect = coordi{{ src_x, src_y }, { src_w, src_h }};
					const areai dst_rect = coordi{{ section.dst_x, section.dst_y }, { section.dst_w, section.dst_h }};
					vk::copy_image_typeless(cmd, section.src, dst, src_rect, dst_rect, 1);

					section.src->pop_layout(cmd);
					continue;
				}

				src_image = vk::get_typeless_helper(dst->format(), dst->format_class(), convert_x + convert_w, src_y + src_h);
				src_image->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

				const areai src_rect = coordi{{ src_x, src_y }, { src_w, src_h }};
				const areai dst_rect = coordi{{ 0, 0 }, { convert_w, src_h }};
				vk::copy_image_typeless(cmd, section.src, src_image, src_rect, dst_rect, 1);
				src_image->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

				src_x = 0;
				src_y = 0;
				src_w = convert_w;
			}

			ensure(src_image->current_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL || src_image->current_layout == VK_IMAGE_LAYOUT_GENERAL);
			ensure(transform == rsx::surface_transform::identity);

			if (src_w == section.dst_w && src_h == section.dst_h) [[likely]]
			{
				const auto copy_rgn = get_output_region(src_x, src_y, src_w, src_h, src_image);
				vkCmdCopyImage(cmd, src_image->value, src_image->current_layout, dst->value, dst->current_layout, 1, &copy_rgn);
			}
			else
			{
				u16 dst_x = section.dst_x, dst_y = section.dst_y;
				vk::image* _dst = dst;

				if (src_image->info.format != dst->info.format || section.level != 0 || section.dst_z != 0) [[ unlikely ]]
				{
					// Either a bitcast is required or a scale+copy to mipmap level / layer
					const u32 requested_width = dst->width();
					const u32 requested_height = src_y + src_h + section.dst_h; // Accounts for possible typeless ref on the same helper on src
					_dst = vk::get_typeless_helper(src_image->format(), src_image->format_class(), requested_width, requested_height);
					_dst->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				}

				if (_dst != dst)
				{
					// We place the output after the source to account for the initial typeless-xfer if applicable
					// If src_image == _dst then this is just a write-to-self. Either way, use best-fit placement.
					dst_x = 0;
					dst_y = src_y + src_h;
				}

				vk::copy_scaled_image(cmd, src_image, _dst,
					coordi{ { src_x, src_y }, { src_w, src_h } },
					coordi{ { dst_x, dst_y }, { section.dst_w, section.dst_h } },
					1, src_image->format() == _dst->format(),
					VK_FILTER_NEAREST);

				if (_dst != dst) [[unlikely]]
				{
					// Casting comes after the scaling!
					const auto copy_rgn = get_output_region(dst_x, dst_y, section.dst_w, section.dst_h, _dst);
					_dst->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
					vkCmdCopyImage(cmd, _dst->value, _dst->current_layout, dst->value, dst->current_layout, 1, &copy_rgn);
				}
			}

			section.src->pop_layout(cmd);
		}
	}

	VkComponentMapping texture_cache::apply_component_mapping_flags(u32 gcm_format, rsx::component_order flags, const rsx::texture_channel_remap_t& remap_vector) const
	{
		switch (gcm_format)
		{
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			// Dont bother letting this propagate
			return{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };
		default:
			break;
		}

		VkComponentMapping mapping = {};
		switch (flags)
		{
		case rsx::component_order::default_:
		{
			mapping = vk::apply_swizzle_remap(vk::get_component_mapping(gcm_format), remap_vector);
			break;
		}
		case rsx::component_order::native:
		{
			mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			break;
		}
		case rsx::component_order::swapped_native:
		{
			mapping = { VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B };
			break;
		}
		default:
			break;
		}

		return mapping;
	}

	vk::image* texture_cache::get_template_from_collection_impl(const std::vector<copy_region_descriptor>& sections_to_transfer) const
	{
		if (sections_to_transfer.size() == 1) [[likely]]
		{
			return sections_to_transfer.front().src;
		}

		vk::image* result = nullptr;
		for (const auto& section : sections_to_transfer)
		{
			if (!section.src)
				continue;

			if (!result)
			{
				result = section.src;
			}
			else
			{
				if (section.src->native_component_map.a != result->native_component_map.a ||
					section.src->native_component_map.r != result->native_component_map.r ||
					section.src->native_component_map.g != result->native_component_map.g ||
					section.src->native_component_map.b != result->native_component_map.b)
				{
					// TODO
					// This requires a far more complex setup as its not always possible to mix and match without compute assistance
					return nullptr;
				}
			}
		}

		return result;
	}

	std::unique_ptr<vk::viewable_image> texture_cache::find_cached_image(VkFormat format, u16 w, u16 h, u16 d, u16 mipmaps, VkImageType type, VkImageCreateFlags create_flags, VkImageUsageFlags usage, VkSharingMode sharing)
	{
		reader_lock lock(m_cached_pool_lock);

		if (!m_cached_images.empty())
		{
			const u64 desired_key = hash_image_properties(format, w, h, d, mipmaps, type, create_flags, sharing);
			lock.upgrade();

			for (auto it = m_cached_images.begin(); it != m_cached_images.end(); ++it)
			{
				if (it->key == desired_key && (it->data->info.usage & usage) == usage)
				{
					auto ret = std::move(it->data);
					m_cached_images.erase(it);
					m_cached_memory_size -= ret->memory->size();
					return ret;
				}
			}
		}

		return {};
	}

	std::unique_ptr<vk::viewable_image> texture_cache::create_temporary_subresource_storage(
		rsx::format_class format_class, VkFormat format,
		u16 width, u16 height, u16 depth, u16 layers, u8 mips,
		VkImageType image_type, VkFlags image_flags, VkFlags usage_flags)
	{
		auto image = find_cached_image(format, width, height, depth, mips, image_type, image_flags, usage_flags, VK_SHARING_MODE_EXCLUSIVE);

		if (!image)
		{
			image = std::make_unique<vk::viewable_image>(*vk::get_current_renderer(), m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image_type,
				format,
				width, height, depth, mips, layers, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, image_flags | VK_IMAGE_CREATE_ALLOW_NULL_RPCS3,
				VMM_ALLOCATION_POOL_TEXTURE_CACHE, format_class);

			if (!image->value)
			{
				// OOM, bail
				return nullptr;
			}
		}

		return image;
	}

	void texture_cache::dispose_reusable_image(std::unique_ptr<vk::viewable_image>& image)
	{
		auto disposable = vk::disposable_t::make(new cached_image_reference_t(this, image));
		vk::get_resource_manager()->dispose(disposable);
	}

	vk::image_view* texture_cache::create_temporary_subresource_view_impl(vk::command_buffer& cmd, vk::image* source, VkImageType image_type, VkImageViewType view_type,
		u32 gcm_format, u16 x, u16 y, u16 w, u16 h, u16 d, u8 mips, const rsx::texture_channel_remap_t& remap_vector, bool copy)
	{
		const VkImageCreateFlags image_flags = (view_type == VK_IMAGE_VIEW_TYPE_CUBE) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
		const VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		const VkFormat dst_format = vk::get_compatible_sampler_format(m_formats_support, gcm_format);
		const u16 layers = (view_type == VK_IMAGE_VIEW_TYPE_CUBE) ? 6 : 1;

		// Provision
		auto image = create_temporary_subresource_storage(rsx::classify_format(gcm_format), dst_format, w, h, d, layers, mips, image_type, image_flags, usage_flags);

		// OOM?
		if (!image)
		{
			return nullptr;
		}

		// This method is almost exclusively used to work on framebuffer resources
		// Keep the original swizzle layout unless there is data format conversion
		VkComponentMapping view_swizzle;
		if (!source || dst_format != source->info.format)
		{
			// This is a data cast operation
			// Use native mapping for the new type
			// TODO: Also simulate the readback+reupload step (very tricky)
			const auto remap = get_component_mapping(gcm_format);
			view_swizzle = { remap[1], remap[2], remap[3], remap[0] };
		}
		else
		{
			view_swizzle = source->native_component_map;
		}

		image->set_debug_name("Temp view");
		image->set_native_component_layout(view_swizzle);
		auto view = image->get_view(remap_vector);

		if (copy)
		{
			std::vector<copy_region_descriptor> region =
			{ {
				.src = source,
				.xform = rsx::surface_transform::coordinate_transform,
				.src_x = x,
				.src_y = y,
				.src_w = w,
				.src_h = h,
				.dst_w = w,
				.dst_h = h
			} };

			vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			copy_transfer_regions_impl(cmd, image.get(), region);
			vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		// TODO: Floating reference. We can do better with some restructuring.
		image.release();
		return view;
	}

	vk::image_view* texture_cache::create_temporary_subresource_view(vk::command_buffer& cmd, vk::image* source, u32 gcm_format,
		u16 x, u16 y, u16 w, u16 h, const rsx::texture_channel_remap_t& remap_vector)
	{
		return create_temporary_subresource_view_impl(cmd, source, source->info.imageType, VK_IMAGE_VIEW_TYPE_2D,
			gcm_format, x, y, w, h, 1, 1, remap_vector, true);
	}

	vk::image_view* texture_cache::create_temporary_subresource_view(vk::command_buffer& cmd, vk::image** source, u32 gcm_format,
		u16 x, u16 y, u16 w, u16 h, const rsx::texture_channel_remap_t& remap_vector)
	{
		return create_temporary_subresource_view(cmd, *source, gcm_format, x, y, w, h, remap_vector);
	}

	vk::image_view* texture_cache::generate_cubemap_from_images(vk::command_buffer& cmd, u32 gcm_format, u16 size,
		const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector)
	{
		auto _template = get_template_from_collection_impl(sections_to_copy);
		auto result = create_temporary_subresource_view_impl(cmd, _template, VK_IMAGE_TYPE_2D,
			VK_IMAGE_VIEW_TYPE_CUBE, gcm_format, 0, 0, size, size, 1, 1, remap_vector, false);

		if (!result)
		{
			// Failed to create temporary object, bail
			return nullptr;
		}

		const auto image = result->image();
		VkImageAspectFlags dst_aspect = vk::get_aspect_flags(result->info.format);
		VkImageSubresourceRange dst_range = { dst_aspect, 0, 1, 0, 6 };
		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_range);

		if (!(dst_aspect & VK_IMAGE_ASPECT_DEPTH_BIT))
		{
			VkClearColorValue clear = {};
			vkCmdClearColorImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
		}
		else
		{
			VkClearDepthStencilValue clear = { 1.f, 0 };
			vkCmdClearDepthStencilImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
		}

		copy_transfer_regions_impl(cmd, image, sections_to_copy);

		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, dst_range);
		return result;
	}

	vk::image_view* texture_cache::generate_3d_from_2d_images(vk::command_buffer& cmd, u32 gcm_format, u16 width, u16 height, u16 depth,
		const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector)
	{
		auto _template = get_template_from_collection_impl(sections_to_copy);
		auto result = create_temporary_subresource_view_impl(cmd, _template, VK_IMAGE_TYPE_3D,
			VK_IMAGE_VIEW_TYPE_3D, gcm_format, 0, 0, width, height, depth, 1, remap_vector, false);

		if (!result)
		{
			// Failed to create temporary object, bail
			return nullptr;
		}

		const auto image = result->image();
		VkImageAspectFlags dst_aspect = vk::get_aspect_flags(result->info.format);
		VkImageSubresourceRange dst_range = { dst_aspect, 0, 1, 0, 1 };
		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_range);

		if (!(dst_aspect & VK_IMAGE_ASPECT_DEPTH_BIT))
		{
			VkClearColorValue clear = {};
			vkCmdClearColorImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
		}
		else
		{
			VkClearDepthStencilValue clear = { 1.f, 0 };
			vkCmdClearDepthStencilImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
		}

		copy_transfer_regions_impl(cmd, image, sections_to_copy);

		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, dst_range);
		return result;
	}

	vk::image_view* texture_cache::generate_atlas_from_images(vk::command_buffer& cmd, u32 gcm_format, u16 width, u16 height,
		const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector)
	{
		auto _template = get_template_from_collection_impl(sections_to_copy);
		auto result = create_temporary_subresource_view_impl(cmd, _template, VK_IMAGE_TYPE_2D,
			VK_IMAGE_VIEW_TYPE_2D, gcm_format, 0, 0, width, height, 1, 1, remap_vector, false);

		if (!result)
		{
			// Failed to create temporary object, bail
			return nullptr;
		}

		const auto image = result->image();
		VkImageAspectFlags dst_aspect = vk::get_aspect_flags(result->info.format);
		VkImageSubresourceRange dst_range = { dst_aspect, 0, 1, 0, 1 };
		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_range);

		if (sections_to_copy[0].dst_w != width || sections_to_copy[0].dst_h != height)
		{
			if (!(dst_aspect & VK_IMAGE_ASPECT_DEPTH_BIT))
			{
				VkClearColorValue clear = {};
				vkCmdClearColorImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
			}
			else
			{
				VkClearDepthStencilValue clear = { 1.f, 0 };
				vkCmdClearDepthStencilImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
			}
		}

		copy_transfer_regions_impl(cmd, image, sections_to_copy);

		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, dst_range);
		return result;
	}

	vk::image_view* texture_cache::generate_2d_mipmaps_from_images(vk::command_buffer& cmd, u32 gcm_format, u16 width, u16 height,
		const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector)
	{
		const auto mipmaps = ::narrow<u8>(sections_to_copy.size());
		auto _template = get_template_from_collection_impl(sections_to_copy);
		auto result = create_temporary_subresource_view_impl(cmd, _template, VK_IMAGE_TYPE_2D,
			VK_IMAGE_VIEW_TYPE_2D, gcm_format, 0, 0, width, height, 1, mipmaps, remap_vector, false);

		if (!result)
		{
			// Failed to create temporary object, bail
			return nullptr;
		}

		const auto image = result->image();
		VkImageAspectFlags dst_aspect = vk::get_aspect_flags(result->info.format);
		VkImageSubresourceRange dst_range = { dst_aspect, 0, mipmaps, 0, 1 };
		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_range);

		if (!(dst_aspect & VK_IMAGE_ASPECT_DEPTH_BIT))
		{
			VkClearColorValue clear = {};
			vkCmdClearColorImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
		}
		else
		{
			VkClearDepthStencilValue clear = { 1.f, 0 };
			vkCmdClearDepthStencilImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
		}

		copy_transfer_regions_impl(cmd, image, sections_to_copy);

		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, dst_range);
		return result;
	}

	void texture_cache::release_temporary_subresource(vk::image_view* view)
	{
		auto resource = dynamic_cast<vk::viewable_image*>(view->image());
		ensure(resource);

		auto image = std::unique_ptr<vk::viewable_image>(resource);
		auto disposable = vk::disposable_t::make(new cached_image_reference_t(this, image));
		vk::get_resource_manager()->dispose(disposable);
	}

	void texture_cache::update_image_contents(vk::command_buffer& cmd, vk::image_view* dst_view, vk::image* src, u16 width, u16 height)
	{
		std::vector<copy_region_descriptor> region =
		{ {
			.src = src,
			.xform = rsx::surface_transform::identity,
			.src_w = width,
			.src_h = height,
			.dst_w = width,
			.dst_h = height
		} };

		auto dst = dst_view->image();
		dst->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copy_transfer_regions_impl(cmd, dst, region);
		dst->pop_layout(cmd);
	}

	cached_texture_section* texture_cache::create_new_texture(vk::command_buffer& cmd, const utils::address_range32& rsx_range, u16 width, u16 height, u16 depth, u16 mipmaps, u32 pitch,
		u32 gcm_format, rsx::texture_upload_context context, rsx::texture_dimension_extended type, bool swizzled, rsx::component_order swizzle_flags, rsx::flags32_t flags)
	{
		const auto section_depth = depth;

		// Define desirable attributes based on type
		VkImageType image_type;
		VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		u8 layer = 0;

		switch (type)
		{
		case rsx::texture_dimension_extended::texture_dimension_1d:
			image_type = VK_IMAGE_TYPE_1D;
			height = 1;
			depth = 1;
			layer = 1;
			break;
		case rsx::texture_dimension_extended::texture_dimension_2d:
			image_type = VK_IMAGE_TYPE_2D;
			depth = 1;
			layer = 1;
			break;
		case rsx::texture_dimension_extended::texture_dimension_cubemap:
			image_type = VK_IMAGE_TYPE_2D;
			depth = 1;
			layer = 6;
			break;
		case rsx::texture_dimension_extended::texture_dimension_3d:
			image_type = VK_IMAGE_TYPE_3D;
			layer = 1;
			break;
		default:
			fmt::throw_exception("Unreachable");
		}

		// Check what actually exists at that address
		const rsx::image_section_attributes_t search_desc = { .gcm_format = gcm_format, .width = width, .height = height, .depth = section_depth, .mipmaps = mipmaps };
		const bool allow_dirty = (context != rsx::texture_upload_context::framebuffer_storage);
		cached_texture_section& region = *find_cached_texture(rsx_range, search_desc, true, true, allow_dirty);
		ensure(!region.is_locked());

		vk::viewable_image* image = nullptr;
		if (region.exists())
		{
			image = dynamic_cast<vk::viewable_image*>(region.get_raw_texture());
			bool reusable = true;

			if (flags & texture_create_flags::do_not_reuse)
			{
				reusable = false;
			}
			else if (flags & texture_create_flags::shareable)
			{
				reusable = (image && image->sharing_mode() == VK_SHARING_MODE_CONCURRENT);
			}

			if (!reusable || !image || region.get_image_type() != type || image->depth() != depth) // TODO
			{
				// Incompatible view/type
				region.destroy();
				image = nullptr;
			}
			else
			{
				ensure(region.is_managed());

				// Reuse
				region.set_rsx_pitch(pitch);

				if (flags & texture_create_flags::initialize_image_contents)
				{
					// Wipe memory
					image->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

					VkImageSubresourceRange range{ image->aspect(), 0, image->mipmaps(), 0, image->layers() };
					if (image->aspect() & VK_IMAGE_ASPECT_COLOR_BIT)
					{
						VkClearColorValue color = { {0.f, 0.f, 0.f, 1.f} };
						vkCmdClearColorImage(cmd, image->value, image->current_layout, &color, 1, &range);
					}
					else
					{
						VkClearDepthStencilValue clear{ 1.f, 255 };
						vkCmdClearDepthStencilImage(cmd, image->value, image->current_layout, &clear, 1, &range);
					}
				}
			}
		}

		if (!image)
		{
			const bool is_cubemap = type == rsx::texture_dimension_extended::texture_dimension_cubemap;
			const VkFormat vk_format = get_compatible_sampler_format(m_formats_support, gcm_format);
			VkImageCreateFlags create_flags = is_cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
			VkSharingMode sharing_mode = (flags & texture_create_flags::shareable) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;

			if (auto found = find_cached_image(vk_format, width, height, depth, mipmaps, image_type, create_flags, usage_flags, sharing_mode))
			{
				image = found.release();
			}
			else
			{
				if (sharing_mode == VK_SHARING_MODE_CONCURRENT)
				{
					create_flags |= VK_IMAGE_CREATE_SHAREABLE_RPCS3;
				}

				image = new vk::viewable_image(*m_device,
					m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					image_type, vk_format,
					width, height, depth, mipmaps, layer, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_TILING_OPTIMAL, usage_flags, create_flags,
					VMM_ALLOCATION_POOL_TEXTURE_CACHE, rsx::classify_format(gcm_format));
			}

			// New section, we must prepare it
			region.reset(rsx_range);
			region.set_gcm_format(gcm_format);
			region.set_image_type(type);
			region.create(width, height, section_depth, mipmaps, image, pitch, true, gcm_format);
		}

		region.set_view_flags(swizzle_flags);
		region.set_context(context);
		region.set_swizzled(swizzled);
		region.set_dirty(false);

		image->native_component_map = apply_component_mapping_flags(gcm_format, swizzle_flags, rsx::default_remap_vector);

		// Its not necessary to lock blit dst textures as they are just reused as necessary
		switch (context)
		{
		case rsx::texture_upload_context::shader_read:
		case rsx::texture_upload_context::blit_engine_src:
			region.protect(utils::protection::ro);
			read_only_range = region.get_min_max(read_only_range, rsx::section_bounds::locked_range);
			break;
		case rsx::texture_upload_context::blit_engine_dst:
			region.set_unpack_swap_bytes(true);
			no_access_range = region.get_min_max(no_access_range, rsx::section_bounds::locked_range);
			break;
		case rsx::texture_upload_context::dma:
		case rsx::texture_upload_context::framebuffer_storage:
			// Should not be initialized with this method
		default:
			fmt::throw_exception("Unexpected upload context 0x%x", u32(context));
		}

		update_cache_tag();
		return &region;
	}

	cached_texture_section* texture_cache::create_nul_section(
		vk::command_buffer& /*cmd*/,
		const utils::address_range32& rsx_range,
		const rsx::image_section_attributes_t& attrs,
		const rsx::GCM_tile_reference& tile,
		bool memory_load)
	{
		auto& region = *find_cached_texture(rsx_range, { .gcm_format = RSX_GCM_FORMAT_IGNORED }, true, false, false);
		ensure(!region.is_locked());

		// Prepare section
		region.reset(rsx_range);
		region.create_dma_only(attrs.width, attrs.height, attrs.pitch);
		region.set_dirty(false);
		region.set_unpack_swap_bytes(true);

		if (memory_load && !tile) // Memory load on DMA tiles will always happen during the actual copy command
		{
			vk::map_dma(rsx_range.start, rsx_range.length());
			vk::load_dma(rsx_range.start, rsx_range.length());
		}

		no_access_range = region.get_min_max(no_access_range, rsx::section_bounds::locked_range);
		update_cache_tag();
		return &region;
	}

	cached_texture_section* texture_cache::upload_image_from_cpu(vk::command_buffer& cmd, const utils::address_range32& rsx_range, u16 width, u16 height, u16 depth, u16 mipmaps, u32 pitch, u32 gcm_format,
		rsx::texture_upload_context context, const std::vector<rsx::subresource_layout>& subresource_layout, rsx::texture_dimension_extended type, bool swizzled)
	{
		if (context != rsx::texture_upload_context::shader_read)
		{
			if (vk::is_renderpass_open(cmd))
			{
				vk::end_renderpass(cmd);
			}
		}

		const bool upload_async = rsx::get_current_renderer()->get_backend_config().supports_asynchronous_compute;
		rsx::flags32_t create_flags = 0;

		if (upload_async && g_fxo->get<AsyncTaskScheduler>().is_host_mode())
		{
			create_flags |= texture_create_flags::do_not_reuse;
			if (m_device->get_graphics_queue() != m_device->get_transfer_queue())
			{
				create_flags |= texture_create_flags::shareable;
			}
		}

		auto section = create_new_texture(cmd, rsx_range, width, height, depth, mipmaps, pitch, gcm_format, context, type, swizzled,
			rsx::component_order::default_, create_flags);

		auto image = section->get_raw_texture();
		image->set_debug_name(fmt::format("Raw Texture @0x%x", rsx_range.start));

		vk::enter_uninterruptible();

		bool input_swizzled = swizzled;
		if (context == rsx::texture_upload_context::blit_engine_src)
		{
			// Swizzling is ignored for blit engine copy and emulated using remapping
			input_swizzled = false;
		}

		rsx::flags32_t upload_command_flags = initialize_image_layout | upload_contents_inline;
		if (context == rsx::texture_upload_context::shader_read && upload_async)
		{
			upload_command_flags |= upload_contents_async;
		}

		std::vector<rsx::subresource_layout> tmp;
		auto p_subresource_layout = &subresource_layout;
		u32 heap_align = upload_heap_align_default;

		if (auto tiled_region = rsx::get_current_renderer()->get_tiled_memory_region(rsx_range);
			context == rsx::texture_upload_context::blit_engine_src && tiled_region)
		{
			if (mipmaps > 1)
			{
				// This really shouldn't happen on framebuffer tiled memory
				rsx_log.error("Tiled decode of mipmapped textures is not supported.");
			}
			else
			{
				const auto bpp = rsx::get_format_block_size_in_bytes(gcm_format);
				const auto [scratch_buf, linear_data_scratch_offset] = vk::detile_memory_block(cmd, tiled_region, rsx_range, width, height, bpp);

				auto subres = subresource_layout.front();
				// FIXME: !!EVIL!!
				subres.data = { scratch_buf, linear_data_scratch_offset };
				subres.pitch_in_block = width;
				upload_command_flags |= source_is_gpu_resident;
				heap_align = width * bpp;

				tmp.push_back(subres);
				p_subresource_layout = &tmp;
			}
		}

		const u16 layer_count = (type == rsx::texture_dimension_extended::texture_dimension_cubemap) ? 6 : 1;
		vk::upload_image(cmd, image, *p_subresource_layout, gcm_format, input_swizzled, layer_count, image->aspect(),
			*m_texture_upload_heap, heap_align, upload_command_flags);

		vk::leave_uninterruptible();

		if (context != rsx::texture_upload_context::shader_read)
		{
			// Insert appropriate barrier depending on use. Shader read resources should be lazy-initialized before consuming.
			// TODO: All texture resources should be initialized on use, this is wasteful

			VkImageLayout preferred_layout;
			switch (context)
			{
			default:
				preferred_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				break;
			case rsx::texture_upload_context::blit_engine_dst:
				preferred_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				break;
			case rsx::texture_upload_context::blit_engine_src:
				preferred_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				break;
			}

			if (preferred_layout != image->current_layout)
			{
				image->change_layout(cmd, preferred_layout);
			}
			else
			{
				// Insert ordering barrier
				ensure(preferred_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				insert_image_memory_barrier(cmd, image->value, image->current_layout, preferred_layout,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
					{ image->aspect(), 0, image->mipmaps(), 0, image->layers() });
			}
		}

		section->last_write_tag = rsx::get_shared_tag();
		return section;
	}

	void texture_cache::set_component_order(cached_texture_section& section, u32 gcm_format, rsx::component_order expected_flags)
	{
		if (expected_flags == section.get_view_flags())
			return;

		const VkComponentMapping mapping = apply_component_mapping_flags(gcm_format, expected_flags, rsx::default_remap_vector);
		auto image = static_cast<vk::viewable_image*>(section.get_raw_texture());

		ensure(image);
		image->set_native_component_layout(mapping);

		section.set_view_flags(expected_flags);
	}

	void texture_cache::insert_texture_barrier(vk::command_buffer& cmd, vk::image* tex, bool strong_ordering)
	{
		if (!strong_ordering && tex->current_layout == VK_IMAGE_LAYOUT_GENERAL)
		{
			// A previous barrier already exists, do nothing
			return;
		}

		vk::as_rtt(tex)->texture_barrier(cmd);
	}

	bool texture_cache::render_target_format_is_compatible(vk::image* tex, u32 gcm_format)
	{
		auto vk_format = tex->info.format;
		switch (gcm_format)
		{
		default:
			//TODO
			err_once("Format incompatibility detected, reporting failure to force data copy (VK_FORMAT=0x%X, GCM_FORMAT=0x%X)", static_cast<u32>(vk_format), gcm_format);
			return false;
#ifndef __APPLE__
		case CELL_GCM_TEXTURE_R5G6B5:
			return (vk_format == VK_FORMAT_R5G6B5_UNORM_PACK16);
#else
		// R5G6B5 is not supported by Metal
		case CELL_GCM_TEXTURE_R5G6B5:
			return (vk_format == VK_FORMAT_B8G8R8A8_UNORM);
#endif
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
			return (vk_format == VK_FORMAT_R16G16B16A16_SFLOAT);
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
			return (vk_format == VK_FORMAT_R32G32B32A32_SFLOAT);
		case CELL_GCM_TEXTURE_X32_FLOAT:
			return (vk_format == VK_FORMAT_R32_SFLOAT);
		case CELL_GCM_TEXTURE_A8R8G8B8:
		case CELL_GCM_TEXTURE_D8R8G8B8:
			return (vk_format == VK_FORMAT_B8G8R8A8_UNORM || vk_format == VK_FORMAT_D24_UNORM_S8_UINT || vk_format == VK_FORMAT_D32_SFLOAT_S8_UINT);
		case CELL_GCM_TEXTURE_B8:
			return (vk_format == VK_FORMAT_R8_UNORM);
		case CELL_GCM_TEXTURE_G8B8:
			return (vk_format == VK_FORMAT_R8G8_UNORM);
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
			return (vk_format == VK_FORMAT_D24_UNORM_S8_UINT || vk_format == VK_FORMAT_D32_SFLOAT_S8_UINT);
		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			return (vk_format == VK_FORMAT_D16_UNORM || vk_format == VK_FORMAT_D32_SFLOAT);
		}
	}

	void texture_cache::prepare_for_dma_transfers(vk::command_buffer& cmd)
	{
		if (!cmd.is_recording())
		{
			cmd.begin();
		}
	}

	void texture_cache::cleanup_after_dma_transfers(vk::command_buffer& cmd)
	{
		bool occlusion_query_active = !!(cmd.flags & vk::command_buffer::cb_has_open_query);
		if (occlusion_query_active)
		{
			// We really stepped in it
			vk::do_query_cleanup(cmd);
		}

		// End recording
		cmd.end();

		if (cmd.access_hint != vk::command_buffer::access_type_hint::all)
		{
			// Flush any pending async jobs in case of blockers
			// TODO: Context-level manager should handle this logic
			auto async_scheduler = g_fxo->try_get<AsyncTaskScheduler>();
			vk::semaphore* async_sema = nullptr;

			if (async_scheduler && async_scheduler->is_recording())
			{
				if (async_scheduler->is_host_mode())
				{
					async_sema = async_scheduler->get_sema();
				}
				else
				{
					vk::queue_submit_t submit_info{};
					async_scheduler->flush(submit_info, VK_TRUE);
				}
			}

			// Primary access command queue, must restart it after
			vk::fence submit_fence(*m_device);
			vk::queue_submit_t submit_info{ m_submit_queue, &submit_fence };

			if (async_sema)
			{
				vk::queue_submit_t submit_info2{};
				submit_info2.queue_signal(*async_sema);
				async_scheduler->flush(submit_info2, VK_TRUE);

				submit_info.wait_on(*async_sema, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			}

			cmd.submit(submit_info, VK_TRUE);
			vk::wait_for_fence(&submit_fence, GENERAL_WAIT_TIMEOUT);

			CHECK_RESULT(vkResetCommandBuffer(cmd, 0));
			cmd.begin();
		}
		else
		{
			// Auxilliary command queue with auto-restart capability
			vk::queue_submit_t submit_info{ m_submit_queue, nullptr };
			cmd.submit(submit_info, VK_TRUE);
		}

		ensure(cmd.flags == 0);

		if (occlusion_query_active)
		{
			ensure(cmd.is_recording());
			cmd.flags |= vk::command_buffer::cb_load_occluson_task;
		}
	}

	void texture_cache::initialize(vk::render_device& device, VkQueue submit_queue, vk::data_heap& upload_heap)
	{
		m_device = &device;
		m_memory_types = device.get_memory_mapping();
		m_formats_support = device.get_formats_support();
		m_submit_queue = submit_queue;
		m_texture_upload_heap = &upload_heap;
	}

	void texture_cache::destroy()
	{
		clear();
	}

	bool texture_cache::is_depth_texture(u32 rsx_address, u32 rsx_size)
	{
		reader_lock lock(m_cache_mutex);

		auto& block = m_storage.block_for(rsx_address);

		if (block.get_locked_count() == 0)
			return false;

		for (auto& tex : block)
		{
			if (tex.is_dirty())
				continue;

			if (!tex.overlaps(rsx_address, rsx::section_bounds::full_range))
				continue;

			if ((rsx_address + rsx_size - tex.get_section_base()) <= tex.get_section_size())
			{
				switch (tex.get_format())
				{
				case VK_FORMAT_D16_UNORM:
				case VK_FORMAT_D32_SFLOAT:
				case VK_FORMAT_D32_SFLOAT_S8_UINT:
				case VK_FORMAT_D24_UNORM_S8_UINT:
					return true;
				default:
					return false;
				}
			}
		}

		//Unreachable; silence compiler warning anyway
		return false;
	}

	bool texture_cache::handle_memory_pressure(rsx::problem_severity severity)
	{
		auto any_released = baseclass::handle_memory_pressure(severity);

		// TODO: This can cause invalidation of in-flight resources
		if (severity <= rsx::problem_severity::low || !m_cached_memory_size)
		{
			// Nothing left to do
			return any_released;
		}

		constexpr u64 _1M = 0x100000;
		if (severity <= rsx::problem_severity::moderate && m_cached_memory_size < (64 * _1M))
		{
			// Some memory is consumed by the temporary resources, but no need to panic just yet
			return any_released;
		}

		std::unique_lock lock(m_cache_mutex, std::defer_lock);
		if (!lock.try_lock())
		{
			rsx_log.warning("Unable to remove temporary resources because we're already in the texture cache!");
			return any_released;
		}

		// Nuke temporary resources. They will still be visible to the GPU.
		auto gc = vk::get_resource_manager();
		any_released |= !m_cached_images.empty();
		for (auto& img : m_cached_images)
		{
			gc->dispose(img.data);
		}
		m_cached_images.clear();
		m_cached_memory_size = 0;

		any_released |= !m_temporary_subresource_cache.empty();
		for (auto& e : m_temporary_subresource_cache)
		{
			ensure(e.second.second);
			release_temporary_subresource(e.second.second);
		}
		m_temporary_subresource_cache.clear();

		return any_released;
	}

	void texture_cache::on_frame_end()
	{
		trim_sections();

		if (m_storage.m_unreleased_texture_objects >= m_max_zombie_objects)
		{
			purge_unreleased_sections();
		}

		if (m_cached_images.size() > max_cached_image_pool_size ||
			m_cached_memory_size > 256 * 0x100000)
		{
			std::lock_guard lock(m_cached_pool_lock);

			const auto new_size = m_cached_images.size() / 2;
			for (usz i = new_size; i < m_cached_images.size(); ++i)
			{
				m_cached_memory_size -= m_cached_images[i].data->memory->size();
			}

			m_cached_images.resize(new_size);
		}

		baseclass::on_frame_end();
		reset_frame_statistics();
	}

	vk::viewable_image* texture_cache::upload_image_simple(vk::command_buffer& cmd, VkFormat format, u32 address, u32 width, u32 height, u32 pitch)
	{
		bool linear_format_supported = false;

		switch (format)
		{
		case VK_FORMAT_B8G8R8A8_UNORM:
			linear_format_supported = m_formats_support.bgra8_linear;
			break;
		case VK_FORMAT_R8G8B8A8_UNORM:
			linear_format_supported = m_formats_support.argb8_linear;
			break;
		default:
			rsx_log.error("Unsupported VkFormat 0x%x", static_cast<u32>(format));
			return nullptr;
		}

		if (!linear_format_supported)
		{
			return nullptr;
		}

		// Uploads a linear memory range as a BGRA8 texture
		auto image = std::make_unique<vk::viewable_image>(*m_device, m_memory_types.host_visible_coherent,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_IMAGE_TYPE_2D,
			format,
			width, height, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED,
			VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0,
			VMM_ALLOCATION_POOL_SWAPCHAIN);

		VkImageSubresource subresource{};
		subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		VkSubresourceLayout layout{};
		vkGetImageSubresourceLayout(*m_device, image->value, &subresource, &layout);

		void* mem = image->memory->map(0, layout.rowPitch * height);

		auto src = vm::_ptr<const char>(address);
		auto dst = static_cast<char*>(mem);

		// TODO: SSE optimization
		for (u32 row = 0; row < height; ++row)
		{
			auto casted_src = reinterpret_cast<const be_t<u32>*>(src);
			auto casted_dst = reinterpret_cast<u32*>(dst);

			for (u32 col = 0; col < width; ++col)
				casted_dst[col] = casted_src[col];

			src += pitch;
			dst += layout.rowPitch;
		}

		image->memory->unmap();

		vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		// Fully dispose immediately. These immages aren't really reusable right now.
		auto result = image.get();
		vk::get_resource_manager()->dispose(image);

		return result;
	}

	bool texture_cache::blit(const rsx::blit_src_info& src, const rsx::blit_dst_info& dst, bool interpolate, vk::surface_cache& m_rtts, vk::command_buffer& cmd)
	{
		blitter helper;
		auto reply = upload_scaled_image(src, dst, interpolate, cmd, m_rtts, helper);

		if (reply.succeeded)
		{
			if (reply.real_dst_size)
			{
				flush_if_cache_miss_likely(cmd, reply.to_address_range());
			}

			return true;
		}

		return false;
	}

	u32 texture_cache::get_unreleased_textures_count() const
	{
		return baseclass::get_unreleased_textures_count() + ::size32(m_cached_images);
	}

	u64 texture_cache::get_temporary_memory_in_use() const
	{
		// TODO: Technically incorrect, we should have separate metrics for cached evictable resources (this value) and temporary active resources.
		return m_cached_memory_size;
	}

	bool texture_cache::is_overallocated() const
	{
		const auto total_device_memory = m_device->get_memory_mapping().device_local_total_bytes / 0x100000;
		u64 quota = 0;

		if (total_device_memory >= 2048)
		{
			quota = std::min<u64>(3072, (total_device_memory * 40) / 100);
		}
		else if (total_device_memory >= 1024)
		{
			quota = std::max<u64>(204, (total_device_memory * 30) / 100);
		}
		else if (total_device_memory >= 768)
		{
			quota = 192;
		}
		else
		{
			quota = std::min<u64>(128, total_device_memory / 2);
		}

		quota *= 0x100000;

		if (const u64 texture_cache_pool_usage = vmm_get_application_pool_usage(VMM_ALLOCATION_POOL_TEXTURE_CACHE);
			texture_cache_pool_usage > quota)
		{
			rsx_log.warning("Texture cache is using %lluM of memory which exceeds the allocation quota of %lluM",
				texture_cache_pool_usage, quota);
			return true;
		}

		return false;
	}
}
