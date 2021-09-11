#pragma once

#include "VKAsyncScheduler.h"
#include "VKDMA.h"
#include "VKRenderTargets.h"
#include "VKResourceManager.h"
#include "VKRenderPass.h"
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
		vk::viewable_image* vram_texture = nullptr;

	public:
		using baseclass::cached_texture_section;

		void create(u16 w, u16 h, u16 depth, u16 mipmaps, vk::image* image, u32 rsx_pitch, bool managed, u32 gcm_format, bool pack_swap_bytes = false)
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

			vk::image* locked_resource = vram_texture;
			u32 transfer_width = width;
			u32 transfer_height = height;
			u32 transfer_x = 0, transfer_y = 0;

			if (context == rsx::texture_upload_context::framebuffer_storage)
			{
				auto surface = vk::as_rtt(vram_texture);
				surface->read_barrier(cmd);
				locked_resource = surface->get_surface(rsx::surface_access::shader_read);
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
			const auto range = (context == rsx::texture_upload_context::framebuffer_storage) ? get_section_range() : get_confirmed_range();
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

		void* map_synchronized(u32, u32)
		{
			return nullptr;
		}

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

		void sync_surface_memory(const std::vector<cached_texture_section*>& surfaces)
		{
			auto rtt = vk::as_rtt(vram_texture);
			rtt->sync_tag();

			for (auto& surface : surfaces)
			{
				rtt->inherit_surface_contents(vk::as_rtt(surface->vram_texture));
			}
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
		enum texture_create_flags : u32
		{
			initialize_image_contents = 1,
		};

		void on_section_destroyed(cached_texture_section& tex) override;

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

		void clear();

		VkComponentMapping apply_component_mapping_flags(u32 gcm_format, rsx::component_order flags, const rsx::texture_channel_remap_t& remap_vector) const;

		void copy_transfer_regions_impl(vk::command_buffer& cmd, vk::image* dst, const std::vector<copy_region_descriptor>& sections_to_transfer) const;

		vk::image* get_template_from_collection_impl(const std::vector<copy_region_descriptor>& sections_to_transfer) const;

		std::unique_ptr<vk::viewable_image> find_temporary_image(VkFormat format, u16 w, u16 h, u16 d, u8 mipmaps);

		std::unique_ptr<vk::viewable_image> find_temporary_cubemap(VkFormat format, u16 size);

	protected:
		vk::image_view* create_temporary_subresource_view_impl(vk::command_buffer& cmd, vk::image* source, VkImageType image_type, VkImageViewType view_type,
			u32 gcm_format, u16 x, u16 y, u16 w, u16 h, u16 d, u8 mips, const rsx::texture_channel_remap_t& remap_vector, bool copy);

		vk::image_view* create_temporary_subresource_view(vk::command_buffer& cmd, vk::image* source, u32 gcm_format,
			u16 x, u16 y, u16 w, u16 h, const rsx::texture_channel_remap_t& remap_vector) override;

		vk::image_view* create_temporary_subresource_view(vk::command_buffer& cmd, vk::image** source, u32 gcm_format,
			u16 x, u16 y, u16 w, u16 h, const rsx::texture_channel_remap_t& remap_vector) override;

		vk::image_view* generate_cubemap_from_images(vk::command_buffer& cmd, u32 gcm_format, u16 size,
			const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector) override;

		vk::image_view* generate_3d_from_2d_images(vk::command_buffer& cmd, u32 gcm_format, u16 width, u16 height, u16 depth,
			const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector) override;

		vk::image_view* generate_atlas_from_images(vk::command_buffer& cmd, u32 gcm_format, u16 width, u16 height,
			const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector) override;

		vk::image_view* generate_2d_mipmaps_from_images(vk::command_buffer& cmd, u32 gcm_format, u16 width, u16 height,
			const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector) override;

		void release_temporary_subresource(vk::image_view* view) override;

		void update_image_contents(vk::command_buffer& cmd, vk::image_view* dst_view, vk::image* src, u16 width, u16 height) override;

		cached_texture_section* create_new_texture(vk::command_buffer& cmd, const utils::address_range& rsx_range, u16 width, u16 height, u16 depth, u16 mipmaps, u16 pitch,
			u32 gcm_format, rsx::texture_upload_context context, rsx::texture_dimension_extended type, bool swizzled, rsx::component_order swizzle_flags, rsx::flags32_t flags) override;

		cached_texture_section* create_nul_section(vk::command_buffer& cmd, const utils::address_range& rsx_range, bool memory_load) override;

		cached_texture_section* upload_image_from_cpu(vk::command_buffer& cmd, const utils::address_range& rsx_range, u16 width, u16 height, u16 depth, u16 mipmaps, u16 pitch, u32 gcm_format,
			rsx::texture_upload_context context, const std::vector<rsx::subresource_layout>& subresource_layout, rsx::texture_dimension_extended type, bool swizzled) override;

		void set_component_order(cached_texture_section& section, u32 gcm_format, rsx::component_order expected_flags) override;

		void insert_texture_barrier(vk::command_buffer& cmd, vk::image* tex, bool strong_ordering) override;

		bool render_target_format_is_compatible(vk::image* tex, u32 gcm_format) override;

		void prepare_for_dma_transfers(vk::command_buffer& cmd) override;

		void cleanup_after_dma_transfers(vk::command_buffer& cmd) override;

	public:
		using baseclass::texture_cache;

		void initialize(vk::render_device& device, VkQueue submit_queue, vk::data_heap& upload_heap);

		void destroy() override;

		bool is_depth_texture(u32 rsx_address, u32 rsx_size) override;

		void on_frame_end() override;

		vk::viewable_image* upload_image_simple(vk::command_buffer& cmd, VkFormat format, u32 address, u32 width, u32 height, u32 pitch);

		bool blit(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate, vk::surface_cache& m_rtts, vk::command_buffer& cmd);

		u32 get_unreleased_textures_count() const override;

		bool handle_memory_pressure(rsx::problem_severity severity) override;

		u32 get_temporary_memory_in_use() const;

		bool is_overallocated() const;
	};
}
