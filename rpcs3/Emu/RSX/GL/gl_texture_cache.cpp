#include "stdafx.h"

#include "gl_texture_cache.h"
#include "GLGSRender.h"
#include "../Common/TextureUtils.h"

#include <exception>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>

namespace gl
{
	texture_cache::~texture_cache()
	{
		clear_obj_cache();
	}

	bool texture_cache::lock_memory_region(u32 start, u32 size)
	{
		static const u32 memory_page_size = 4096;
		start = start & ~(memory_page_size - 1);
		size = (u32)align(size, memory_page_size);

		return vm::page_protect(start, size, 0, 0, vm::page_writable);
	}

	bool texture_cache::unlock_memory_region(u32 start, u32 size)
	{
		static const u32 memory_page_size = 4096;
		start = start & ~(memory_page_size - 1);
		size = (u32)align(size, memory_page_size);

		return vm::page_protect(start, size, 0, vm::page_writable, 0);
	}

	void texture_cache::lock_gl_object(cached_texture &obj)
	{
		static const u32 memory_page_size = 4096;
		obj.protected_block_start = obj.data_addr & ~(memory_page_size - 1);
		obj.protected_block_sz = (u32)align(obj.block_sz, memory_page_size);

		if (!lock_memory_region(obj.protected_block_start, obj.protected_block_sz))
			LOG_ERROR(RSX, "lock_gl_object failed!");
		else
			obj.locked = true;
	}

	void texture_cache::unlock_gl_object(cached_texture &obj)
	{
		if (!unlock_memory_region(obj.protected_block_start, obj.protected_block_sz))
			LOG_ERROR(RSX, "unlock_gl_object failed! Will probably crash soon...");
		else
			obj.locked = false;
	}

	cached_texture *texture_cache::find_obj_for_params(u64 texaddr, u32 w, u32 h, u16 mipmap)
	{
		for (cached_texture &tex : m_texture_cache)
		{
			if (tex.gl_id && tex.data_addr == texaddr)
			{
				if (w && h && mipmap && (tex.h != h || tex.w != w || tex.mipmap != mipmap))
				{
					LOG_ERROR(RSX, "Texture params are invalid for block starting 0x%X!", tex.data_addr);
					LOG_ERROR(RSX, "Params passed w=%d, h=%d, mip=%d, found w=%d, h=%d, mip=%d", w, h, mipmap, tex.w, tex.h, tex.mipmap);

					continue;
				}

				tex.frame_ctr = m_frame_ctr;
				return &tex;
			}
		}

		return nullptr;
	}

	cached_texture& texture_cache::create_obj_for_params(u32 gl_id, u64 texaddr, u32 w, u32 h, u16 mipmap)
	{
		cached_texture obj = { 0 };

		obj.gl_id = gl_id;
		obj.data_addr = texaddr;
		obj.w = w;
		obj.h = h;
		obj.mipmap = mipmap;
		obj.deleted = false;
		obj.locked = false;

		for (cached_texture &tex : m_texture_cache)
		{
			if (tex.gl_id == 0 || (tex.deleted && (m_frame_ctr - tex.frame_ctr) > 32768))
			{
				if (tex.gl_id)
				{
					LOG_NOTICE(RSX, "Reclaiming GL texture %d, cache_size=%d, master_ctr=%d, ctr=%d", tex.gl_id, m_texture_cache.size(), m_frame_ctr, tex.frame_ctr);
					__glcheck glDeleteTextures(1, &tex.gl_id);
					unlock_gl_object(tex);
					tex.gl_id = 0;
				}

				tex = obj;
				return tex;
			}
		}

		m_texture_cache.push_back(obj);
		return m_texture_cache.back();
	}

	void texture_cache::remove_obj(cached_texture &tex)
	{
		if (tex.locked)
			unlock_gl_object(tex);

		tex.deleted = true;
	}

	void texture_cache::remove_obj_for_glid(u32 gl_id)
	{
		for (cached_texture &tex : m_texture_cache)
		{
			if (tex.gl_id == gl_id)
				remove_obj(tex);
		}
	}

	void texture_cache::clear_obj_cache()
	{
		for (cached_texture &tex : m_texture_cache)
		{
			if (tex.locked)
				unlock_gl_object(tex);

			if (tex.gl_id)
			{
				LOG_NOTICE(RSX, "Deleting texture %d", tex.gl_id);
				glDeleteTextures(1, &tex.gl_id);
			}

			tex.deleted = true;
			tex.gl_id = 0;
		}

		m_texture_cache.clear();
		destroy_rtt_cache();
	}

	bool texture_cache::region_overlaps(u32 base1, u32 limit1, u32 base2, u32 limit2)
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

	cached_rtt* texture_cache::find_cached_rtt(u32 base, u32 size)
	{
		for (cached_rtt &rtt : m_rtt_cache)
		{
			if (region_overlaps(base, base + size, rtt.data_addr, rtt.data_addr + rtt.block_sz))
			{
				return &rtt;
			}
		}

		return nullptr;
	}

	void texture_cache::invalidate_rtts_in_range(u32 base, u32 size)
	{
		for (cached_rtt &rtt : m_rtt_cache)
		{
			if (!rtt.data_addr || rtt.is_dirty) continue;

			u32 rtt_aligned_base = ((u32)(rtt.data_addr)) & ~(4096 - 1);
			u32 rtt_block_sz = align(rtt.block_sz, 4096);

			if (region_overlaps(rtt_aligned_base, (rtt_aligned_base + rtt_block_sz), base, base + size))
			{
				LOG_NOTICE(RSX, "Dirty RTT FOUND addr=0x%X", base);
				rtt.is_dirty = true;
				if (rtt.locked)
				{
					rtt.locked = false;
					unlock_memory_region((u32)rtt.data_addr, rtt.block_sz);
				}
			}
		}
	}

	void texture_cache::prep_rtt(cached_rtt &rtt, u32 width, u32 height, u32 gl_pixel_format_internal)
	{
		int binding = 0;
		bool is_depth = false;

		if (gl_pixel_format_internal == GL_DEPTH24_STENCIL8 ||
			gl_pixel_format_internal == GL_DEPTH_COMPONENT24 ||
			gl_pixel_format_internal == GL_DEPTH_COMPONENT16 ||
			gl_pixel_format_internal == GL_DEPTH_COMPONENT32)
		{
			is_depth = true;
		}

		glGetIntegerv(GL_TEXTURE_2D_BINDING_EXT, &binding);
		glBindTexture(GL_TEXTURE_2D, rtt.copy_glid);

		rtt.current_width = width;
		rtt.current_height = height;

		if (!is_depth)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			__glcheck glTexImage2D(GL_TEXTURE_2D, 0, gl_pixel_format_internal, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			u32 ex_format = GL_UNSIGNED_SHORT;
			u32 in_format = GL_DEPTH_COMPONENT16;

			switch (gl_pixel_format_internal)
			{
			case GL_DEPTH24_STENCIL8:
			{
				ex_format = GL_UNSIGNED_INT_24_8;
				in_format = GL_DEPTH_STENCIL;
				break;
			}
			case GL_DEPTH_COMPONENT16:
				break;
			default:
				throw EXCEPTION("Unsupported depth format!");
			}

			__glcheck glTexImage2D(GL_TEXTURE_2D, 0, gl_pixel_format_internal, width, height, 0, in_format, ex_format, nullptr);
		}

		glBindTexture(GL_TEXTURE_2D, binding);
		rtt.is_depth = is_depth;
	}

	void texture_cache::save_rtt(u32 base, u32 size, u32 width, u32 height, u32 gl_pixel_format_internal, gl::texture &source)
	{
		cached_rtt *region = find_cached_rtt(base, size);

		if (!region)
		{
			for (cached_rtt &rtt : m_rtt_cache)
			{
				if (rtt.valid && rtt.data_addr == 0)
				{
					prep_rtt(rtt, width, height, gl_pixel_format_internal);

					rtt.block_sz = size;
					rtt.data_addr = base;
					rtt.is_dirty = true;

					LOG_NOTICE(RSX, "New RTT created for block 0x%X + 0x%X", (u32)rtt.data_addr, rtt.block_sz);

					lock_memory_region((u32)rtt.data_addr, rtt.block_sz);
					rtt.locked = true;

					region = &rtt;
					break;
				}
			}

			if (!region) throw EXCEPTION("No region created!!");
		}

		if (width != region->current_width ||
			height != region->current_height)
		{
			prep_rtt(*region, width, height, gl_pixel_format_internal);

			if (region->locked && region->block_sz != size)
			{
				LOG_NOTICE(RSX, "Unlocking RTT since size has changed!");
				unlock_memory_region((u32)region->data_addr, region->block_sz);

				LOG_NOTICE(RSX, "Locking down RTT after size change!");
				region->block_sz = size;
				lock_memory_region((u32)region->data_addr, region->block_sz);
				region->locked = true;
			}
		}

		__glcheck glCopyImageSubData(source.id(), GL_TEXTURE_2D, 0, 0, 0, 0,
			region->copy_glid, GL_TEXTURE_2D, 0, 0, 0, 0,
			width, height, 1);

		region->is_dirty = false;

		if (!region->locked)
		{
			LOG_WARNING(RSX, "Locking down RTT, was unlocked!");
			lock_memory_region((u32)region->data_addr, region->block_sz);
			region->locked = true;
		}
	}

	void texture_cache::write_rtt(u32 base, u32 size, u32 texaddr)
	{
		//Actually download the data, since it seems that cell is writing to it manually
		throw;
	}

	void texture_cache::destroy_rtt_cache()
	{
		for (cached_rtt &rtt : m_rtt_cache)
		{
			rtt.valid = false;
			rtt.is_dirty = false;
			rtt.block_sz = 0;
			rtt.data_addr = 0;

			glDeleteTextures(1, &rtt.copy_glid);
			rtt.copy_glid = 0;
		}

		m_rtt_cache.clear();
	}

	void texture_cache::update_frame_ctr()
	{
		m_frame_ctr++;
	}

	void texture_cache::initialize_rtt_cache()
	{
		if (!m_rtt_cache.empty())
		{
			throw EXCEPTION("Initialize RTT cache while cache already exists! Leaking objects??");
		}

		for (int i = 0; i < 64; ++i)
		{
			cached_rtt rtt;

			glGenTextures(1, &rtt.copy_glid);
			rtt.is_dirty = true;
			rtt.valid = true;
			rtt.block_sz = 0;
			rtt.data_addr = 0;
			rtt.locked = false;

			m_rtt_cache.push_back(rtt);
		}
	}

	void texture_cache::upload_texture(int index, rsx::texture &tex, rsx::gl::texture &gl_texture)
	{
		const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
		const u32 range = (u32)get_texture_size(tex);

		cached_rtt *rtt = find_cached_rtt(texaddr, range);

		if (rtt && !rtt->is_dirty)
		{
			if (!rtt->is_depth)
			{
				u32 real_id = gl_texture.id();

				glActiveTexture(GL_TEXTURE0 + index);
				gl_texture.set_id(rtt->copy_glid);
				gl_texture.bind();

				gl_texture.set_id(real_id);
			}
			else
			{
				LOG_NOTICE(RSX, "Depth RTT found from 0x%X, Trying to upload width dims: %d x %d, Saved as %d x %d", rtt->data_addr, tex.width(), tex.height(), rtt->current_width, rtt->current_height);
				//The texture should have already been loaded through the writeback interface call
				//Bind it directly
				u32 real_id = gl_texture.id();

				glActiveTexture(GL_TEXTURE0 + index);
				gl_texture.set_id(rtt->copy_glid);
				gl_texture.bind();

				gl_texture.set_id(real_id);
			}
			return;
		}
		else if (rtt)
			LOG_NOTICE(RSX, "RTT texture for address 0x%X is dirty!", texaddr);

		cached_texture *obj = nullptr;

		if (!rtt)
			obj = find_obj_for_params(texaddr, tex.width(), tex.height(), tex.mipmap());

		if (obj && !obj->deleted)
		{
			u32 real_id = gl_texture.id();

			glActiveTexture(GL_TEXTURE0 + index);
			gl_texture.set_id(obj->gl_id);
			gl_texture.bind();

			gl_texture.set_id(real_id);
		}
		else
		{
			if (!obj) gl_texture.set_id(0);
			else
			{
				//Reuse this GLid
				gl_texture.set_id(obj->gl_id);

				//Empty this slot for another one. A new holder will be created below anyway...
				if (obj->locked) unlock_gl_object(*obj);
				obj->gl_id = 0;
			}

			__glcheck gl_texture.init(index, tex);
			cached_texture &_obj = create_obj_for_params(gl_texture.id(), texaddr, tex.width(), tex.height(), tex.mipmap());

			_obj.block_sz = (u32)get_texture_size(tex);
			lock_gl_object(_obj);
		}
	}

	bool texture_cache::mark_as_dirty(u32 address)
	{
		bool response = false;

		for (cached_texture &tex : m_texture_cache)
		{
			if (!tex.locked) continue;

			if (tex.protected_block_start <= address &&
				tex.protected_block_sz > (address - tex.protected_block_start))
			{
				LOG_NOTICE(RSX, "Texture object is dirty! %d", tex.gl_id);
				unlock_gl_object(tex);

				invalidate_rtts_in_range((u32)tex.data_addr, tex.block_sz);

				tex.deleted = true;
				response = true;
			}
		}

		if (response) return true;

		for (cached_rtt &rtt : m_rtt_cache)
		{
			if (!rtt.data_addr || rtt.is_dirty) continue;

			u32 rtt_aligned_base = ((u32)(rtt.data_addr)) & ~(4096 - 1);
			u32 rtt_block_sz = align(rtt.block_sz, 4096);

			if (rtt.locked && (u64)address >= rtt_aligned_base)
			{
				u32 offset = address - rtt_aligned_base;
				if (offset >= rtt_block_sz) continue;

				LOG_NOTICE(RSX, "Dirty non-texture RTT FOUND! addr=0x%X", rtt.data_addr);
				rtt.is_dirty = true;

				unlock_memory_region(rtt_aligned_base, rtt_block_sz);
				rtt.locked = false;

				response = true;
			}
		}

		return response;
	}

	void texture_cache::save_render_target(u32 texaddr, u32 range, gl::texture &gl_texture)
	{
		save_rtt(texaddr, range, gl_texture.width(), gl_texture.height(), (GLenum)gl_texture.get_internal_format(), gl_texture);
	}

	std::vector<invalid_cache_area> texture_cache::find_and_invalidate_in_range(u32 base, u32 limit)
	{
		/**
		* Sometimes buffers can share physical pages.
		* Return objects if we really encroach on texture
		*/

		std::vector<invalid_cache_area> result;

		for (cached_texture &obj : m_texture_cache)
		{
			//Check for memory area overlap. unlock page(s) if needed and add this index to array.
			//Axis separation test
			const u32 &block_start = obj.protected_block_start;
			const u32 block_end = block_start + obj.protected_block_sz;

			if (limit < block_start) continue;
			if (base > block_end) continue;

			u32 min_separation = (limit - base) + obj.protected_block_sz;
			u32 range_limit = (block_end > limit) ? block_end : limit;
			u32 range_base = (block_start < base) ? block_start : base;

			u32 actual_separation = (range_limit - range_base);

			if (actual_separation < min_separation)
			{
				const u32 texture_start = (u32)obj.data_addr;
				const u32 texture_end = texture_start + obj.block_sz;

				min_separation = (limit - base) + obj.block_sz;
				range_limit = (texture_end > limit) ? texture_end : limit;
				range_base = (texture_start < base) ? texture_start : base;

				actual_separation = (range_limit - range_base);

				if (actual_separation < min_separation)
				{
					//Texture area is invalidated!
					unlock_gl_object(obj);
					obj.deleted = true;

					continue;
				}

				//Overlap in this case will be at most 1 page...
				invalid_cache_area invalid = { 0 };
				if (base < obj.data_addr)
					invalid.block_base = obj.protected_block_start;
				else
					invalid.block_base = obj.protected_block_start + obj.protected_block_sz - 4096;

				invalid.block_sz = 4096;
				unlock_memory_region(invalid.block_base, invalid.block_sz);
				result.push_back(invalid);
			}
		}

		return result;
	}

	void texture_cache::lock_invalidated_ranges(std::vector<invalid_cache_area> invalid)
	{
		for (invalid_cache_area area : invalid)
		{
			lock_memory_region(area.block_base, area.block_sz);
		}
	}

	void texture_cache::remove_in_range(u32 texaddr, u32 range)
	{
		//Seems that the rsx only 'reads' full texture objects..
		//This simplifies this function to simply check for matches
		for (cached_texture &cached : m_texture_cache)
		{
			if (cached.data_addr == texaddr &&
				cached.block_sz == range)
				remove_obj(cached);
		}
	}

	bool texture_cache::explicit_writeback(gl::texture &tex, const u32 address, const u32 pitch)
	{
		const u32 range = tex.height() * pitch;
		cached_rtt *rtt = find_cached_rtt(address, range);

		if (rtt && !rtt->is_dirty)
		{
			u32 min_w = rtt->current_width;
			u32 min_h = rtt->current_height;

			if ((u32)tex.width() < min_w) min_w = (u32)tex.width();
			if ((u32)tex.height() < min_h) min_h = (u32)tex.height();

			//TODO: Image reinterpretation e.g read back rgba data as depth texture and vice-versa

			__glcheck glCopyImageSubData(rtt->copy_glid, GL_TEXTURE_2D, 0, 0, 0, 0,
				tex.id(), GL_TEXTURE_2D, 0, 0, 0, 0,
				min_w, min_h, 1);

			return true;
		}

		//No valid object found in cache
		return false;
	}
}
