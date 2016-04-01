#pragma once
#include "stdafx.h"
#include "VKRenderTargets.h"
#include "VKGSRender.h"
#include "../Common/TextureUtils.h"

namespace vk
{
	struct cached_texture_object
	{
		u32 native_rsx_address;
		u32 native_rsx_size;
		
		u16 width;
		u16 height;
		u16 depth;
		u16 mipmaps;
		std::unique_ptr<vk::image_view> uploaded_image_view;
		std::unique_ptr<vk::image> uploaded_texture;

		u64  protected_rgn_start;
		u64  protected_rgn_end;
		
		bool exists = false;
		bool locked = false;
		bool dirty = true;
	};

	class texture_cache
	{
	private:
		std::vector<cached_texture_object> m_cache;

		std::vector<std::unique_ptr<vk::image_view> > m_temporary_image_view;

		bool lock_memory_region(u32 start, u32 size)
		{
			static const u32 memory_page_size = 4096;
			start = start & ~(memory_page_size - 1);
			size = (u32)align(size, memory_page_size);

			return vm::page_protect(start, size, 0, 0, vm::page_writable);
		}

		bool unlock_memory_region(u32 start, u32 size)
		{
			static const u32 memory_page_size = 4096;
			start = start & ~(memory_page_size - 1);
			size = (u32)align(size, memory_page_size);

			return vm::page_protect(start, size, 0, vm::page_writable, 0);
		}

		bool region_overlaps(u32 base1, u32 limit1, u32 base2, u32 limit2)
		{
			//Check for memory area overlap. unlock page(s) if needed and add this index to array.
			//Axis separation test
			const u32 &block_start = base1;
			const u32 block_end = limit1;

			if (limit2 < block_start) return false;
			if (base2 > block_end) return false;

			u32 min_separation = (limit2 - base2) + (limit1 - base1);
			u32 range_limit = (block_end > limit2) ? block_end : limit2;
			u32 range_base = (block_start < base2) ? block_start : base2;

			u32 actual_separation = (range_limit - range_base);

			if (actual_separation < min_separation)
				return true;

			return false;
		}

		cached_texture_object& find_cached_texture(u32 rsx_address, u32 rsx_size, bool confirm_dimensions = false, u16 width = 0, u16 height = 0, u16 mipmaps = 0)
		{
			for (cached_texture_object &tex : m_cache)
			{
				if (!tex.dirty && tex.exists &&
					tex.native_rsx_address == rsx_address &&
					tex.native_rsx_size == rsx_size)
				{
					if (!confirm_dimensions) return tex;

					if (tex.width == width && tex.height == height && tex.mipmaps == mipmaps)
						return tex;
					else
					{
						LOG_ERROR(RSX, "Cached object for address 0x%X was found, but it does not match stored parameters.");
						LOG_ERROR(RSX, "%d x %d vs %d x %d", width, height, tex.width, tex.height);
					}
				}
			}

			for (cached_texture_object &tex : m_cache)
			{
				if (tex.dirty)
				{
					if (tex.exists)
					{
						tex.uploaded_texture.reset();
						tex.exists = false;
					}

					return tex;
				}
			}

			m_cache.push_back(cached_texture_object());

			return m_cache[m_cache.size() - 1];
		}

		void lock_object(cached_texture_object &obj)
		{
			static const u32 memory_page_size = 4096;
			obj.protected_rgn_start = obj.native_rsx_address & ~(memory_page_size - 1);
			obj.protected_rgn_end = (u32)align(obj.native_rsx_size, memory_page_size);
			obj.protected_rgn_end += obj.protected_rgn_start;

			lock_memory_region(static_cast<u32>(obj.protected_rgn_start), static_cast<u32>(obj.native_rsx_size));
		}

		void unlock_object(cached_texture_object &obj)
		{
			unlock_memory_region(static_cast<u32>(obj.protected_rgn_start), static_cast<u32>(obj.native_rsx_size));
		}

		void purge_dirty_textures()
		{
			for (cached_texture_object &tex : m_cache)
			{
				if (tex.dirty && tex.exists)
				{
					tex.uploaded_texture.reset();
					tex.exists = false;
				}
			}
		}

	public:

		texture_cache() {}
		~texture_cache() {}

		void destroy()
		{
			m_cache.resize(0);
		}

		vk::image_view* upload_texture(command_buffer cmd, rsx::texture &tex, rsx::vk_render_targets &m_rtts, const vk::memory_type_mapping &memory_type_mapping, vk_data_heap& upload_heap, vk::buffer* upload_buffer)
		{
			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
			const u32 range = (u32)get_texture_size(tex);

			//First check if it exists as an rtt...
			vk::image *rtt_texture = nullptr;
			if (rtt_texture = m_rtts.get_texture_from_render_target_if_applicable(texaddr))
			{
				m_temporary_image_view.push_back(std::make_unique<vk::image_view>(*vk::get_current_renderer(), rtt_texture->value, VK_IMAGE_VIEW_TYPE_2D, rtt_texture->info.format,
					vk::default_component_map(),
					vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT)));
				return m_temporary_image_view.back().get();
			}

			if (rtt_texture = m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr))
			{
				m_temporary_image_view.push_back(std::make_unique<vk::image_view>(*vk::get_current_renderer(), rtt_texture->value, VK_IMAGE_VIEW_TYPE_2D, rtt_texture->info.format,
					vk::default_component_map(),
					vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_DEPTH_BIT)));
				return m_temporary_image_view.back().get();
			}

			cached_texture_object& cto = find_cached_texture(texaddr, range, true, tex.width(), tex.height(), tex.get_exact_mipmap_count());
			if (cto.exists && !cto.dirty)
			{
				return cto.uploaded_image_view.get();
			}

			u32 raw_format = tex.format();
			u32 format = raw_format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);

			VkComponentMapping mapping = vk::get_component_mapping(format, tex.remap());
			VkFormat vk_format = get_compatible_sampler_format(format);

			VkImageType image_type;
			VkImageViewType image_view_type;
			u16 height;
			u16 depth;
			u8 layer;
			switch (tex.get_extended_texture_dimension())
			{
			case rsx::texture_dimension_extended::texture_dimension_1d:
				image_type = VK_IMAGE_TYPE_1D;
				image_view_type = VK_IMAGE_VIEW_TYPE_1D;
				height = 1;
				depth = 1;
				layer = 1;
				break;
			case rsx::texture_dimension_extended::texture_dimension_2d:
				image_type = VK_IMAGE_TYPE_2D;
				image_view_type = VK_IMAGE_VIEW_TYPE_2D;
				height = tex.height();
				depth = 1;
				layer = 1;
				break;
			case rsx::texture_dimension_extended::texture_dimension_cubemap:
				image_type = VK_IMAGE_TYPE_2D;
				image_view_type = VK_IMAGE_VIEW_TYPE_CUBE;
				height = tex.height();
				depth = 1;
				layer = 6;
				break;
			case rsx::texture_dimension_extended::texture_dimension_3d:
				image_type = VK_IMAGE_TYPE_3D;
				image_view_type = VK_IMAGE_VIEW_TYPE_3D;
				depth = tex.depth();
				layer = 1;
				break;
			}

			bool is_cubemap = tex.get_extended_texture_dimension() == rsx::texture_dimension_extended::texture_dimension_cubemap;
			VkImageSubresourceRange subresource_range = vk::get_image_subresource_range(0, 0, is_cubemap ? 6 : 1, tex.get_exact_mipmap_count(), VK_IMAGE_ASPECT_COLOR_BIT);

			cto.uploaded_texture = std::make_unique<vk::image>(*vk::get_current_renderer(), memory_type_mapping.device_local,
				image_type,
				vk_format,
				tex.width(), height, depth, tex.get_exact_mipmap_count(), layer, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, is_cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0);
			change_image_layout(cmd, cto.uploaded_texture->value, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);

			cto.uploaded_image_view = std::make_unique<vk::image_view>(*vk::get_current_renderer(), cto.uploaded_texture->value, image_view_type, vk_format,
				vk::get_component_mapping(tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN), tex.remap()),
				subresource_range);

			copy_mipmaped_image_using_buffer(cmd, cto.uploaded_texture->value, get_subresources_layout(tex), format, !(tex.format() & CELL_GCM_TEXTURE_LN), tex.get_exact_mipmap_count(),
				upload_heap, upload_buffer);

			change_image_layout(cmd, cto.uploaded_texture->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresource_range);

			cto.exists = true;
			cto.dirty = false;
			cto.native_rsx_address = texaddr;
			cto.native_rsx_size = range;
			cto.width = tex.width();
			cto.height = tex.height();
			cto.mipmaps = tex.get_exact_mipmap_count();
			
			lock_object(cto);

			return cto.uploaded_image_view.get();
		}

		bool invalidate_address(u32 rsx_address)
		{
			for (cached_texture_object &tex : m_cache)
			{
				if (tex.dirty) continue;

				if (rsx_address >= tex.protected_rgn_start &&
					rsx_address < tex.protected_rgn_end)
				{
					unlock_object(tex);

					tex.native_rsx_address = 0;
					tex.dirty = true;

					return true;
				}
			}

			return false;
		}

		void flush()
		{
			m_temporary_image_view.clear();
		}
	};
}
