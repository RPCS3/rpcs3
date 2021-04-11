#pragma once

#include "VKAsyncScheduler.h"
#include "VKDMA.h"
#include "VKRenderTargets.h"
#include "VKResourceManager.h"
#include "vkutils/image_helpers.h"

#include "../Common/texture_cache.h"
#include "Emu/Cell/timers.hpp"

#include <memory>
#include <vector>

namespace vk
{
	class cached_texture_section;
	class texture_cache;

	struct texture_cache_traits
	{
		using commandbuffer_type      = vk::command_buffer;
		using section_storage_type    = vk::cached_texture_section;
		using texture_cache_type      = vk::texture_cache;
		using texture_cache_base_type = rsx::texture_cache<texture_cache_type, texture_cache_traits>;
		using image_resource_type     = vk::image*;
		using image_view_type         = vk::image_view*;
		using image_storage_type      = vk::image;
		using texture_format          = VkFormat;
	};

	class cached_texture_section : public rsx::cached_texture_section<vk::cached_texture_section, vk::texture_cache_traits>
	{
		using baseclass = typename rsx::cached_texture_section<vk::cached_texture_section, vk::texture_cache_traits>;
		friend baseclass;

		std::unique_ptr<vk::viewable_image> managed_texture = nullptr;

		//DMA relevant data
		std::unique_ptr<vk::event> dma_fence;
		vk::render_device* m_device = nullptr;
		vk::viewable_image *vram_texture = nullptr;

	public:
		using baseclass::cached_texture_section;

		void create(u16 w, u16 h, u16 depth, u16 mipmaps, vk::image *image, u32 rsx_pitch, bool managed, u32 gcm_format, bool pack_swap_bytes = false)
		{
			auto new_texture = static_cast<vk::viewable_image*>(image);
			ensure(!exists() || !is_managed() || vram_texture == new_texture);
			vram_texture = new_texture;

			ensure(rsx_pitch);

			width = w;
			height = h;
			this->depth = depth;
			this->mipmaps = mipmaps;
			this->rsx_pitch = rsx_pitch;

			this->gcm_format = gcm_format;
			this->pack_unpack_swap_bytes = pack_swap_bytes;

			if (managed)
			{
				managed_texture.reset(vram_texture);
			}

			if (auto rtt = dynamic_cast<vk::render_target*>(image))
			{
				swizzled = (rtt->raster_type != rsx::surface_raster_type::linear);
			}

			if (synchronized)
			{
				// Even if we are managing the same vram section, we cannot guarantee contents are static
				// The create method is only invoked when a new managed session is required
				release_dma_resources();
				synchronized = false;
				flushed = false;
				sync_timestamp = 0ull;
			}

			// Notify baseclass
			baseclass::on_section_resources_created();
		}

		void release_dma_resources()
		{
			if (dma_fence)
			{
				auto gc = vk::get_resource_manager();
				gc->dispose(dma_fence);
			}
		}

		void dma_abort() override
		{
			// Called if a reset occurs, usually via reprotect path after a bad prediction.
			// Discard the sync event, the next sync, if any, will properly recreate this.
			ensure(synchronized);
			ensure(!flushed);
			ensure(dma_fence);
			vk::get_resource_manager()->dispose(dma_fence);
		}

		void destroy()
		{
			if (!exists() && context != rsx::texture_upload_context::dma)
				return;

			m_tex_cache->on_section_destroyed(*this);

			vram_texture = nullptr;
			ensure(!managed_texture);
			release_dma_resources();

			baseclass::on_section_resources_destroyed();
		}

		bool exists() const
		{
			return (vram_texture != nullptr);
		}

		bool is_managed() const
		{
			return !exists() || managed_texture;
		}

		vk::image_view* get_view(u32 remap_encoding, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap)
		{
			ensure(vram_texture != nullptr);
			return vram_texture->get_view(remap_encoding, remap);
		}

		vk::image_view* get_raw_view()
		{
			ensure(vram_texture != nullptr);
			return vram_texture->get_view(0xAAE4, rsx::default_remap_vector);
		}

		vk::image* get_raw_texture()
		{
			return managed_texture.get();
		}

		std::unique_ptr<vk::viewable_image>& get_texture()
		{
			return managed_texture;
		}

		VkFormat get_format() const
		{
			if (context == rsx::texture_upload_context::dma)
			{
				return VK_FORMAT_R32_UINT;
			}

			ensure(vram_texture != nullptr);
			return vram_texture->format();
		}

		bool is_flushed() const
		{
			//This memory section was flushable, but a flush has already removed protection
			return flushed;
		}

		void dma_transfer(vk::command_buffer& cmd, vk::image* src, const areai& src_area, const utils::address_range& valid_range, u32 pitch);

		void copy_texture(vk::command_buffer& cmd, bool miss)
		{
			ensure(exists());

			if (!miss) [[likely]]
			{
				ensure(!synchronized);
				baseclass::on_speculative_flush();
			}
			else
			{
				baseclass::on_miss();
			}

			if (m_device == nullptr)
			{
				m_device = &cmd.get_command_pool().get_owner();
			}

			vk::image *locked_resource = vram_texture;
			u32 transfer_width = width;
			u32 transfer_height = height;
			u32 transfer_x = 0, transfer_y = 0;

			if (context == rsx::texture_upload_context::framebuffer_storage)
			{
				auto surface = vk::as_rtt(vram_texture);
				surface->read_barrier(cmd);
				locked_resource = surface->get_surface(rsx::surface_access::read);
				transfer_width *= surface->samples_x;
				transfer_height *= surface->samples_y;
			}

			vk::image* target = locked_resource;
			if (transfer_width != locked_resource->width() || transfer_height != locked_resource->height())
			{
				// TODO: Synchronize access to typeles textures
				target = vk::get_typeless_helper(vram_texture->format(), vram_texture->format_class(), transfer_width, transfer_height);
				target->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

				// Allow bilinear filtering on color textures where compatibility is likely
				const auto filter = (target->aspect() == VK_IMAGE_ASPECT_COLOR_BIT) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;

				vk::copy_scaled_image(cmd, locked_resource, target,
					{ 0, 0, static_cast<s32>(locked_resource->width()), static_cast<s32>(locked_resource->height()) },
					{ 0, 0, static_cast<s32>(transfer_width), static_cast<s32>(transfer_height) },
					1, true, filter);

				target->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			}

			const auto internal_bpp = vk::get_format_texel_width(vram_texture->format());
			const auto valid_range = get_confirmed_range();

			if (const auto section_range = get_section_range(); section_range != valid_range)
			{
				if (const auto offset = (valid_range.start - get_section_base()))
				{
					transfer_y = offset / rsx_pitch;
					transfer_x = (offset % rsx_pitch) / internal_bpp;

					ensure(transfer_width >= transfer_x);
					ensure(transfer_height >= transfer_y);
					transfer_width -= transfer_x;
					transfer_height -= transfer_y;
				}

				if (const auto tail = (section_range.end - valid_range.end))
				{
					const auto row_count = tail / rsx_pitch;

					ensure(transfer_height >= row_count);
					transfer_height -= row_count;
				}
			}

			areai src_area;
			src_area.x1 = static_cast<s32>(transfer_x);
			src_area.y1 = static_cast<s32>(transfer_y);
			src_area.x2 = s32(transfer_x + transfer_width);
			src_area.y2 = s32(transfer_y + transfer_height);
			dma_transfer(cmd, target, src_area, valid_range, rsx_pitch);
		}

		/**
		 * Flush
		 */
		void imp_flush() override
		{
			AUDIT(synchronized);

			// Synchronize, reset dma_fence after waiting
			vk::wait_for_event(dma_fence.get(), GENERAL_WAIT_TIMEOUT);

			// Calculate smallest range to flush - for framebuffers, the raster region is enough
			const auto range = (context == rsx::texture_upload_context::framebuffer_storage)? get_section_range() : get_confirmed_range();
			vk::flush_dma(range.start, range.length());

			if (is_swizzled())
			{
				// This format is completely worthless to CPU processing algorithms where cache lines on die are linear.
				// If this is happening, usually it means it was not a planned readback (e.g shared pages situation)
				rsx_log.warning("[Performance warning] CPU readback of swizzled data");

				// Read-modify-write to avoid corrupting already resident memory outside texture region
				void* data = get_ptr(range.start);
				std::vector<u8> tmp_data(rsx_pitch * height);
				std::memcpy(tmp_data.data(), data, tmp_data.size());

				switch (gcm_format)
				{
				case CELL_GCM_TEXTURE_A8R8G8B8:
				case CELL_GCM_TEXTURE_DEPTH24_D8:
					rsx::convert_linear_swizzle<u32, false>(tmp_data.data(), data, width, height, rsx_pitch);
					break;
				case CELL_GCM_TEXTURE_R5G6B5:
				case CELL_GCM_TEXTURE_DEPTH16:
					rsx::convert_linear_swizzle<u16, false>(tmp_data.data(), data, width, height, rsx_pitch);
					break;
				default:
					rsx_log.error("Unexpected swizzled texture format 0x%x", gcm_format);
				}
			}

			if (context == rsx::texture_upload_context::framebuffer_storage)
			{
				// Update memory tag
				static_cast<vk::render_target*>(vram_texture)->sync_tag();
			}
		}

		void *map_synchronized(u32, u32)
		{ return nullptr; }

		void finish_flush()
		{}

		/**
		 * Misc
		 */
		void set_unpack_swap_bytes(bool swap_bytes)
		{
			pack_unpack_swap_bytes = swap_bytes;
		}

		void set_rsx_pitch(u16 pitch)
		{
			ensure(!is_locked());
			rsx_pitch = pitch;
		}

		bool is_synchronized() const
		{
			return synchronized;
		}

		bool has_compatible_format(vk::image* tex) const
		{
			return vram_texture->info.format == tex->info.format;
		}

		bool is_depth_texture() const
		{
			switch (vram_texture->info.format)
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
	};

	struct temporary_storage
	{
		std::unique_ptr<vk::viewable_image> combined_image;
		bool can_reuse = false;

		// Memory held by this temp storage object
		u32 block_size = 0;

		// Frame id tag
		const u64 frame_tag = vk::get_current_frame_id();

		temporary_storage(std::unique_ptr<vk::viewable_image>& _img)
		{
			combined_image = std::move(_img);
		}

		temporary_storage(vk::cached_texture_section& tex)
		{
			combined_image = std::move(tex.get_texture());
			block_size = tex.get_section_size();
		}

		bool test(u64 ref_frame) const
		{
			return ref_frame > 0 && frame_tag <= ref_frame;
		}

		bool matches(VkFormat format, u16 w, u16 h, u16 d, u16 mipmaps, VkFlags flags) const
		{
			if (combined_image &&
				combined_image->info.flags == flags &&
				combined_image->format() == format &&
				combined_image->width() == w &&
				combined_image->height() == h &&
				combined_image->depth() == d &&
				combined_image->mipmaps() == mipmaps)
			{
				return true;
			}

			return false;
		}
	};

	class texture_cache : public rsx::texture_cache<vk::texture_cache, vk::texture_cache_traits>
	{
	private:
		using baseclass = rsx::texture_cache<vk::texture_cache, vk::texture_cache_traits>;
		friend baseclass;

	public:
		void on_section_destroyed(cached_texture_section& tex) override
		{
			if (tex.is_managed())
			{
				vk::get_resource_manager()->dispose(tex.get_texture());
			}
		}

	private:

		//Vulkan internals
		vk::render_device* m_device;
		vk::memory_type_mapping m_memory_types;
		vk::gpu_formats_support m_formats_support;
		VkQueue m_submit_queue;
		vk::data_heap* m_texture_upload_heap;

		//Stuff that has been dereferenced goes into these
		std::list<temporary_storage> m_temporary_storage;
		atomic_t<u32> m_temporary_memory_size = { 0 };

		void clear()
		{
			baseclass::clear();

			m_temporary_storage.clear();
			m_temporary_memory_size = 0;
		}

		VkComponentMapping apply_component_mapping_flags(u32 gcm_format, rsx::texture_create_flags flags, const rsx::texture_channel_remap_t& remap_vector) const
		{
			switch (gcm_format)
			{
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
				//Dont bother letting this propagate
				return{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };
			default:
				break;
			}

			VkComponentMapping mapping = {};
			switch (flags)
			{
			case rsx::texture_create_flags::default_component_order:
			{
				mapping = vk::apply_swizzle_remap(vk::get_component_mapping(gcm_format), remap_vector);
				break;
			}
			case rsx::texture_create_flags::native_component_order:
			{
				mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
				break;
			}
			case rsx::texture_create_flags::swapped_native_component_order:
			{
				mapping = { VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B };
				break;
			}
			default:
				break;
			}

			return mapping;
		}

		void copy_transfer_regions_impl(vk::command_buffer& cmd, vk::image* dst, const std::vector<copy_region_descriptor>& sections_to_transfer) const;

		vk::image* get_template_from_collection_impl(const std::vector<copy_region_descriptor>& sections_to_transfer) const
		{
			if (sections_to_transfer.size() == 1) [[likely]]
			{
				return sections_to_transfer.front().src;
			}

			vk::image* result = nullptr;
			for (const auto &section : sections_to_transfer)
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

		std::unique_ptr<vk::viewable_image> find_temporary_image(VkFormat format, u16 w, u16 h, u16 d, u8 mipmaps)
		{
			//const auto current_frame = vk::get_current_frame_id();
			for (auto &e : m_temporary_storage)
			{
				if (e.can_reuse && e.matches(format, w, h, d, mipmaps, 0))
				{
					m_temporary_memory_size -= e.block_size;
					e.block_size = 0;
					return std::move(e.combined_image);
				}
			}

			return {};
		}

		std::unique_ptr<vk::viewable_image> find_temporary_cubemap(VkFormat format, u16 size)
		{
			//const auto current_frame = vk::get_current_frame_id();
			for (auto &e : m_temporary_storage)
			{
				if (e.can_reuse && e.matches(format, size, size, 1, 1, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT))
				{
					m_temporary_memory_size -= e.block_size;
					e.block_size = 0;
					return std::move(e.combined_image);
				}
			}

			return {};
		}

	protected:
		vk::image_view* create_temporary_subresource_view_impl(vk::command_buffer& cmd, vk::image* source, VkImageType image_type, VkImageViewType view_type,
			u32 gcm_format, u16 x, u16 y, u16 w, u16 h, u16 d, u8 mips, const rsx::texture_channel_remap_t& remap_vector, bool copy)
		{
			std::unique_ptr<vk::viewable_image> image;

			VkImageCreateFlags image_flags = (view_type == VK_IMAGE_VIEW_TYPE_CUBE) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
			VkFormat dst_format = vk::get_compatible_sampler_format(m_formats_support, gcm_format);
			u16 layers = 1;

			if (!image_flags) [[likely]]
			{
				image = find_temporary_image(dst_format, w, h, 1, mips);
			}
			else
			{
				image = find_temporary_cubemap(dst_format, w);
				layers = 6;
			}

			if (!image)
			{
				image = std::make_unique<vk::viewable_image>(*vk::get_current_renderer(), m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					image_type,
					dst_format,
					w, h, d, mips, layers, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, image_flags,
					rsx::classify_format(gcm_format));
			}

			//This method is almost exclusively used to work on framebuffer resources
			//Keep the original swizzle layout unless there is data format conversion
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

			image->set_native_component_layout(view_swizzle);
			auto view = image->get_view(rsx::get_remap_encoding(remap_vector), remap_vector);

			if (copy)
			{
				std::vector<copy_region_descriptor> region =
				{{
					source,
					rsx::surface_transform::coordinate_transform,
					0,
					x, y, 0, 0, 0,
					w, h, w, h
				}};

				vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				copy_transfer_regions_impl(cmd, image.get(), region);
				vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}

			const u32 resource_memory = w * h * 4; //Rough approximate
			m_temporary_storage.emplace_back(image);
			m_temporary_storage.back().block_size = resource_memory;
			m_temporary_memory_size += resource_memory;

			return view;
		}

		vk::image_view* create_temporary_subresource_view(vk::command_buffer& cmd, vk::image* source, u32 gcm_format,
				u16 x, u16 y, u16 w, u16 h, const rsx::texture_channel_remap_t& remap_vector) override
		{
			return create_temporary_subresource_view_impl(cmd, source, source->info.imageType, VK_IMAGE_VIEW_TYPE_2D,
					gcm_format, x, y, w, h, 1, 1, remap_vector, true);
		}

		vk::image_view* create_temporary_subresource_view(vk::command_buffer& cmd, vk::image** source, u32 gcm_format,
				u16 x, u16 y, u16 w, u16 h, const rsx::texture_channel_remap_t& remap_vector) override
		{
			return create_temporary_subresource_view(cmd, *source, gcm_format, x, y, w, h, remap_vector);
		}

		vk::image_view* generate_cubemap_from_images(vk::command_buffer& cmd, u32 gcm_format, u16 size,
				const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector) override
		{
			auto _template = get_template_from_collection_impl(sections_to_copy);
			auto result = create_temporary_subresource_view_impl(cmd, _template, VK_IMAGE_TYPE_2D,
				VK_IMAGE_VIEW_TYPE_CUBE, gcm_format, 0, 0, size, size, 1, 1, remap_vector, false);

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

		vk::image_view* generate_3d_from_2d_images(vk::command_buffer& cmd, u32 gcm_format, u16 width, u16 height, u16 depth,
			const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector) override
		{
			auto _template = get_template_from_collection_impl(sections_to_copy);
			auto result = create_temporary_subresource_view_impl(cmd, _template, VK_IMAGE_TYPE_3D,
				VK_IMAGE_VIEW_TYPE_3D, gcm_format, 0, 0, width, height, depth, 1, remap_vector, false);

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

		vk::image_view* generate_atlas_from_images(vk::command_buffer& cmd, u32 gcm_format, u16 width, u16 height,
				const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector) override
		{
			auto _template = get_template_from_collection_impl(sections_to_copy);
			auto result = create_temporary_subresource_view_impl(cmd, _template, VK_IMAGE_TYPE_2D,
					VK_IMAGE_VIEW_TYPE_2D, gcm_format, 0, 0, width, height, 1, 1, remap_vector, false);

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

		vk::image_view* generate_2d_mipmaps_from_images(vk::command_buffer& cmd, u32 gcm_format, u16 width, u16 height,
			const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector) override
		{
			const auto mipmaps = ::narrow<u8>(sections_to_copy.size());
			auto _template = get_template_from_collection_impl(sections_to_copy);
			auto result = create_temporary_subresource_view_impl(cmd, _template, VK_IMAGE_TYPE_2D,
				VK_IMAGE_VIEW_TYPE_2D, gcm_format, 0, 0, width, height, 1, mipmaps, remap_vector, false);

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

		void release_temporary_subresource(vk::image_view* view) override
		{
			auto handle = dynamic_cast<vk::viewable_image*>(view->image());
			for (auto& e : m_temporary_storage)
			{
				if (e.combined_image.get() == handle)
				{
					e.can_reuse = true;
					return;
				}
			}
		}

		void update_image_contents(vk::command_buffer& cmd, vk::image_view* dst_view, vk::image* src, u16 width, u16 height) override
		{
			std::vector<copy_region_descriptor> region =
			{ {
				src,
				rsx::surface_transform::identity,
				0,
				0, 0, 0, 0, 0,
				width, height, width, height
			}};

			auto dst = dst_view->image();
			dst->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			copy_transfer_regions_impl(cmd, dst, region);
			dst->pop_layout(cmd);
		}

		cached_texture_section* create_new_texture(vk::command_buffer& cmd, const utils::address_range &rsx_range, u16 width, u16 height, u16 depth, u16 mipmaps, u16 pitch,
			u32 gcm_format, rsx::texture_upload_context context, rsx::texture_dimension_extended type, bool swizzled, rsx::texture_create_flags flags) override
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
				if (!image || region.get_image_type() != type || image->depth() != depth) // TODO
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

					if (context != rsx::texture_upload_context::shader_read)
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

				image = new vk::viewable_image(*m_device, m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					image_type,
					vk_format,
					width, height, depth, mipmaps, layer, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_TILING_OPTIMAL, usage_flags, is_cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
					rsx::classify_format(gcm_format));

				// New section, we must prepare it
				region.reset(rsx_range);
				region.set_gcm_format(gcm_format);
				region.set_image_type(type);
				region.create(width, height, section_depth, mipmaps, image, pitch, true, gcm_format);
			}

			region.set_view_flags(flags);
			region.set_context(context);
			region.set_swizzled(swizzled);
			region.set_dirty(false);

			image->native_component_map = apply_component_mapping_flags(gcm_format, flags, rsx::default_remap_vector);

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
				// Should not initialized with this method
			default:
				fmt::throw_exception("Unexpected upload context 0x%x", u32(context));
			}

			update_cache_tag();
			return &region;
		}

		cached_texture_section* create_nul_section(vk::command_buffer& /*cmd*/, const utils::address_range& rsx_range, bool memory_load) override
		{
			auto& region = *find_cached_texture(rsx_range, { .gcm_format = RSX_GCM_FORMAT_IGNORED }, true, false, false);
			ensure(!region.is_locked());

			// Prepare section
			region.reset(rsx_range);
			region.set_context(rsx::texture_upload_context::dma);
			region.set_dirty(false);
			region.set_unpack_swap_bytes(true);

			if (memory_load)
			{
				vk::map_dma(rsx_range.start, rsx_range.length());
				vk::load_dma(rsx_range.start, rsx_range.length());
			}

			no_access_range = region.get_min_max(no_access_range, rsx::section_bounds::locked_range);
			update_cache_tag();
			return &region;
		}

		cached_texture_section* upload_image_from_cpu(vk::command_buffer& cmd, const utils::address_range& rsx_range, u16 width, u16 height, u16 depth, u16 mipmaps, u16 pitch, u32 gcm_format,
			rsx::texture_upload_context context, const std::vector<rsx::subresource_layout>& subresource_layout, rsx::texture_dimension_extended type, bool swizzled) override
		{
			auto section = create_new_texture(cmd, rsx_range, width, height, depth, mipmaps, pitch, gcm_format, context, type, swizzled,
					rsx::texture_create_flags::default_component_order);

			auto image = section->get_raw_texture();
			image->set_debug_name(fmt::format("Raw Texture @0x%x", rsx_range.start));

			vk::enter_uninterruptible();

			bool input_swizzled = swizzled;
			if (context == rsx::texture_upload_context::blit_engine_src)
			{
				// Swizzling is ignored for blit engine copy and emulated using remapping
				input_swizzled = false;
			}

			rsx::flags32_t upload_command_flags = initialize_image_layout |
				(g_cfg.video.vk.asynchronous_texture_streaming? upload_contents_async : upload_contents_inline);

			vk::upload_image(cmd, image, subresource_layout, gcm_format, input_swizzled, mipmaps, image->aspect(),
				*m_texture_upload_heap, upload_heap_align_default, upload_command_flags);

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

		void enforce_surface_creation_type(cached_texture_section& section, u32 gcm_format, rsx::texture_create_flags expected_flags) override
		{
			if (expected_flags == section.get_view_flags())
				return;

			const VkComponentMapping mapping = apply_component_mapping_flags(gcm_format, expected_flags, rsx::default_remap_vector);
			auto image = static_cast<vk::viewable_image*>(section.get_raw_texture());

			ensure(image);
			image->set_native_component_layout(mapping);

			section.set_view_flags(expected_flags);
		}

		void insert_texture_barrier(vk::command_buffer& cmd, vk::image* tex, bool strong_ordering) override
		{
			if (!strong_ordering && tex->current_layout == VK_IMAGE_LAYOUT_GENERAL)
			{
				// A previous barrier already exists, do nothing
				return;
			}

			vk::as_rtt(tex)->texture_barrier(cmd);
		}

		bool render_target_format_is_compatible(vk::image* tex, u32 gcm_format) override
		{
			auto vk_format = tex->info.format;
			switch (gcm_format)
			{
			default:
				//TODO
				warn_once("Format incompatibility detected, reporting failure to force data copy (VK_FORMAT=0x%X, GCM_FORMAT=0x%X)", static_cast<u32>(vk_format), gcm_format);
				return false;
			case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
				return (vk_format == VK_FORMAT_R16G16B16A16_SFLOAT);
			case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
				return (vk_format == VK_FORMAT_R32G32B32A32_SFLOAT);
			case CELL_GCM_TEXTURE_X32_FLOAT:
				return (vk_format == VK_FORMAT_R32_SFLOAT);
			case CELL_GCM_TEXTURE_R5G6B5:
				return (vk_format == VK_FORMAT_R5G6B5_UNORM_PACK16);
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
				return (vk_format == VK_FORMAT_D16_UNORM);
			}
		}

		void prepare_for_dma_transfers(vk::command_buffer& cmd) override
		{
			if (!cmd.is_recording())
			{
				cmd.begin();
			}
		}

		void cleanup_after_dma_transfers(vk::command_buffer& cmd) override
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
				g_fxo->get<async_scheduler_thread>().flush(VK_TRUE);

				// Primary access command queue, must restart it after
				vk::fence submit_fence(*m_device);
				cmd.submit(m_submit_queue, VK_NULL_HANDLE, VK_NULL_HANDLE, &submit_fence, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_TRUE);

				vk::wait_for_fence(&submit_fence, GENERAL_WAIT_TIMEOUT);

				CHECK_RESULT(vkResetCommandBuffer(cmd, 0));
				cmd.begin();
			}
			else
			{
				// Auxilliary command queue with auto-restart capability
				cmd.submit(m_submit_queue, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_TRUE);
			}

			ensure(cmd.flags == 0);

			if (occlusion_query_active)
			{
				ensure(cmd.is_recording());
				cmd.flags |= vk::command_buffer::cb_load_occluson_task;
			}
		}

	public:
		using baseclass::texture_cache;

		void initialize(vk::render_device& device, VkQueue submit_queue, vk::data_heap& upload_heap)
		{
			m_device = &device;
			m_memory_types = device.get_memory_mapping();
			m_formats_support = device.get_formats_support();
			m_submit_queue = submit_queue;
			m_texture_upload_heap = &upload_heap;
		}

		void destroy() override
		{
			clear();
		}

		bool is_depth_texture(u32 rsx_address, u32 rsx_size) override
		{
			reader_lock lock(m_cache_mutex);

			auto &block = m_storage.block_for(rsx_address);

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

		void on_frame_end() override
		{
			trim_sections();

			if (m_storage.m_unreleased_texture_objects >= m_max_zombie_objects ||
				m_temporary_memory_size > 0x4000000) //If already holding over 64M in discardable memory, be frugal with memory resources
			{
				purge_unreleased_sections();
			}

			const u64 last_complete_frame = vk::get_last_completed_frame_id();
			m_temporary_storage.remove_if([&](const temporary_storage& o)
			{
				if (!o.block_size || o.test(last_complete_frame))
				{
					m_temporary_memory_size -= o.block_size;
					return true;
				}
				return false;
			});

			m_temporary_subresource_cache.clear();
			reset_frame_statistics();

			baseclass::on_frame_end();
		}

		vk::image *upload_image_simple(vk::command_buffer& cmd, VkFormat format, u32 address, u32 width, u32 height, u32 pitch)
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
				VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0);

			VkImageSubresource subresource{};
			subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			VkSubresourceLayout layout{};
			vkGetImageSubresourceLayout(*m_device, image->value, &subresource, &layout);

			void* mem = image->memory->map(0, layout.rowPitch * height);

			auto src = vm::_ptr<const char>(address);
			auto dst = static_cast<char*>(mem);

			//TODO: SSE optimization
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

			auto result = image.get();
			const u32 resource_memory = width * height * 4; //Rough approximate
			m_temporary_storage.emplace_back(image);
			m_temporary_storage.back().block_size = resource_memory;
			m_temporary_memory_size += resource_memory;

			return result;
		}

		bool blit(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate, rsx::vk_render_targets& m_rtts, vk::command_buffer& cmd)
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

		u32 get_unreleased_textures_count() const override
		{
			return baseclass::get_unreleased_textures_count() + ::size32(m_temporary_storage);
		}

		u32 get_temporary_memory_in_use()
		{
			return m_temporary_memory_size;
		}
	};
}
