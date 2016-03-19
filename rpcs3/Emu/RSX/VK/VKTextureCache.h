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
		
		vk::texture uploaded_texture;

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
		u32 num_dirty_textures = 0;

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
						tex.uploaded_texture.destroy();
						tex.exists = false;
					}

					return tex;
				}
			}

			cached_texture_object object;
			m_cache.push_back(object);

			return m_cache[m_cache.size() - 1];
		}

		void lock_object(cached_texture_object &obj)
		{
			static const u32 memory_page_size = 4096;
			obj.protected_rgn_start = obj.native_rsx_address & ~(memory_page_size - 1);
			obj.protected_rgn_end = (u32)align(obj.native_rsx_size, memory_page_size);
			obj.protected_rgn_end += obj.protected_rgn_start;

			lock_memory_region(obj.protected_rgn_start, obj.native_rsx_size);
		}

		void unlock_object(cached_texture_object &obj)
		{
			unlock_memory_region(obj.protected_rgn_start, obj.native_rsx_size);
		}

		void purge_dirty_textures()
		{
			for (cached_texture_object &tex : m_cache)
			{
				if (tex.dirty && tex.exists)
				{
					tex.uploaded_texture.destroy();
					tex.exists = false;
				}
			}

			num_dirty_textures = 0;
		}

	public:

		texture_cache() {}
		~texture_cache() {}

		void destroy()
		{
			for (cached_texture_object &tex : m_cache)
			{
				if (tex.exists)
				{
					tex.uploaded_texture.destroy();
					tex.exists = false;
				}
			}

			m_cache.resize(0);
		}

		vk::texture& upload_texture(command_buffer cmd, rsx::texture &tex, rsx::vk_render_targets &m_rtts)
		{
			if (num_dirty_textures > 32)
			{
				/**
				 * Should actually reuse available dirty textures whenever possible.
				 * For now, just remove them, from vram
				 */
				purge_dirty_textures();
			}

			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
			const u32 range = (u32)get_texture_size(tex);

			//First check if it exists as an rtt...
			vk::texture *rtt_texture = nullptr;
			if (rtt_texture = m_rtts.get_texture_from_render_target_if_applicable(texaddr))
			{
				return *rtt_texture;
			}

			if (rtt_texture = m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr))
			{
				return *rtt_texture;
			}

			cached_texture_object& cto = find_cached_texture(texaddr, range, true, tex.width(), tex.height(), tex.mipmap());
			if (cto.exists && !cto.dirty)
			{
				return cto.uploaded_texture;
			}

			u32 raw_format = tex.format();
			u32 format = raw_format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);

			VkComponentMapping mapping = vk::get_component_mapping(format, tex.remap());
			VkFormat vk_format = get_compatible_sampler_format(format);

			cto.uploaded_texture.create(*vk::get_current_renderer(), vk_format, VK_IMAGE_USAGE_SAMPLED_BIT, tex.width(), tex.height(), tex.mipmap(), false, mapping);
			cto.uploaded_texture.init(tex, cmd);
			cto.uploaded_texture.change_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			cto.exists = true;
			cto.dirty = false;
			cto.native_rsx_address = texaddr;
			cto.native_rsx_size = range;
			cto.width = cto.uploaded_texture.width();
			cto.height = cto.uploaded_texture.height();
			cto.mipmaps = cto.uploaded_texture.mipmaps();
			
			lock_object(cto);

			return cto.uploaded_texture;
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

					num_dirty_textures++;
					tex.native_rsx_address = 0;
					tex.dirty = true;

					return true;
				}
			}

			return false;
		}

		void flush(vk::command_buffer &cmd)
		{
			//Finish all pending transactions for any cache managed textures..
			for (cached_texture_object &tex : m_cache)
			{
				if (tex.dirty || !tex.exists) continue;
				tex.uploaded_texture.flush(cmd);
			}
		}

		void merge_dirty_textures(std::list<vk::texture> dirty_textures)
		{
			for (vk::texture &tex : dirty_textures)
			{
				cached_texture_object cto;
				cto.uploaded_texture = tex;
				cto.locked = false;
				cto.exists = true;
				cto.dirty = true;
				cto.native_rsx_address = 0;

				num_dirty_textures++;
				m_cache.push_back(cto);
			}
		}
	};
}
