#pragma once

#include "stdafx.h"

#include <exception>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>

#include "GLGSRender.h"
#include "gl_render_targets.h"
#include "../Common/TextureUtils.h"
#include <chrono>

namespace gl
{
	class gl_texture_cache
	{
	public:

		struct gl_cached_texture
		{
			u32 gl_id;
			u32 w;
			u32 h;
			u64 data_addr;
			u32 block_sz;
			u32 frame_ctr;
			u32 protected_block_start;
			u32 protected_block_sz;
			u16 mipmap;
			bool deleted;
			bool locked;
		};

		struct invalid_cache_area
		{
			u32 block_base;
			u32 block_sz;
		};

		struct cached_rtt
		{
			u32 copy_glid;
			u32 data_addr;
			u32 block_sz;

			bool is_dirty;
			bool is_depth;
			bool valid;

			u32 current_width;
			u32 current_height;

			bool locked;
			cached_rtt() : valid(false) {}
		};

	private:
		std::vector<gl_cached_texture> texture_cache;
		std::vector<cached_rtt> rtt_cache;
		u32 frame_ctr;
		std::pair<u64, u64> texture_cache_range = std::make_pair(0xFFFFFFFF, 0);
		u32 max_tex_address = 0;

		bool lock_memory_region(u32 start, u32 size)
		{
			static const u32 memory_page_size = 4096;
			start = start & ~(memory_page_size - 1);
			size = (u32)align(size, memory_page_size);

			if (start < texture_cache_range.first)
				texture_cache_range = std::make_pair(start, texture_cache_range.second);

			if ((start+size) > texture_cache_range.second)
				texture_cache_range = std::make_pair(texture_cache_range.first, (start+size));

			return vm::page_protect(start, size, 0, 0, vm::page_writable);
		}

		bool unlock_memory_region(u32 start, u32 size)
		{
			static const u32 memory_page_size = 4096;
			start = start & ~(memory_page_size - 1);
			size = (u32)align(size, memory_page_size);

			return vm::page_protect(start, size, 0, vm::page_writable, 0);
		}

		void lock_gl_object(gl_cached_texture &obj)
		{
			static const u32 memory_page_size = 4096;
			obj.protected_block_start = obj.data_addr & ~(memory_page_size - 1);
			obj.protected_block_sz = (u32)align(obj.block_sz, memory_page_size);

			if (!lock_memory_region(obj.protected_block_start, obj.protected_block_sz))
				LOG_ERROR(RSX, "lock_gl_object failed!");
			else
				obj.locked = true;
		}

		void unlock_gl_object(gl_cached_texture &obj)
		{
			if (!unlock_memory_region(obj.protected_block_start, obj.protected_block_sz))
				LOG_ERROR(RSX, "unlock_gl_object failed! Will probably crash soon...");
			else
				obj.locked = false;
		}

		gl_cached_texture *find_obj_for_params(u64 texaddr, u32 w, u32 h, u16 mipmap)
		{
			for (gl_cached_texture &tex: texture_cache)
			{
				if (tex.gl_id && tex.data_addr == texaddr)
				{
					if (w && h && mipmap && (tex.h != h || tex.w != w || tex.mipmap != mipmap))
					{
						continue;
					}

					tex.frame_ctr = frame_ctr;
					return &tex;
				}
			}

			return nullptr;
		}

		gl_cached_texture& create_obj_for_params(u32 gl_id, u64 texaddr, u32 w, u32 h, u16 mipmap)
		{
			gl_cached_texture obj = { 0 };
			
			obj.gl_id = gl_id;
			obj.data_addr = texaddr;
			obj.w = w;
			obj.h = h;
			obj.mipmap = mipmap;
			obj.deleted = false;
			obj.locked = false;

			for (gl_cached_texture &tex : texture_cache)
			{
				if (tex.gl_id == 0 || (tex.deleted && (frame_ctr - tex.frame_ctr) > 32768))
				{
					if (tex.gl_id)
					{
						LOG_NOTICE(RSX, "Reclaiming GL texture %d, cache_size=%d, master_ctr=%d, ctr=%d", tex.gl_id, texture_cache.size(), frame_ctr, tex.frame_ctr);
						__glcheck glDeleteTextures(1, &tex.gl_id);
						unlock_gl_object(tex);
						tex.gl_id = 0;
					}

					tex = obj;
					return tex;
				}
			}

			texture_cache.push_back(obj);
			return texture_cache[texture_cache.size()-1];
		}

		void remove_obj(gl_cached_texture &tex)
		{
			if (tex.locked)
				unlock_gl_object(tex);

			tex.deleted = true;
		}

		void remove_obj_for_glid(u32 gl_id)
		{
			for (gl_cached_texture &tex : texture_cache)
			{
				if (tex.gl_id == gl_id)
					remove_obj(tex);
			}
		}

		void clear_obj_cache()
		{
			for (gl_cached_texture &tex : texture_cache)
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

			texture_cache.resize(0);
			destroy_rtt_cache();
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

		cached_rtt* find_cached_rtt(u32 base, u32 size)
		{
			for (cached_rtt &rtt : rtt_cache)
			{
				if (region_overlaps(base, base+size, rtt.data_addr, rtt.data_addr+rtt.block_sz))
				{
					return &rtt;
				}
			}

			return nullptr;
		}

		void invalidate_rtts_in_range(u32 base, u32 size)
		{
			for (cached_rtt &rtt : rtt_cache)
			{
				if (!rtt.data_addr || rtt.is_dirty) continue;

				u32 rtt_aligned_base = ((u32)(rtt.data_addr)) & ~(4096 - 1);
				u32 rtt_block_sz = align(rtt.block_sz, 4096);

				if (region_overlaps(rtt_aligned_base, (rtt_aligned_base + rtt_block_sz), base, base+size))
				{
					rtt.is_dirty = true;
					if (rtt.locked)
					{
						rtt.locked = false;
						unlock_memory_region((u32)rtt.data_addr, rtt.block_sz);
					}
				}
			}
		}

		void prep_rtt(cached_rtt &rtt, u32 width, u32 height, u32 gl_pixel_format_internal)
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

		void save_rtt(u32 base, u32 size, u32 width, u32 height, u32 gl_pixel_format_internal, gl::texture &source)
		{
			cached_rtt *region = find_cached_rtt(base, size);

			if (!region)
			{
				for (cached_rtt &rtt : rtt_cache)
				{
					if (rtt.valid && rtt.data_addr == 0)
					{
						prep_rtt(rtt, width, height, gl_pixel_format_internal);
						
						rtt.block_sz = size;
						rtt.data_addr = base;
						rtt.is_dirty = true;

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
					unlock_memory_region((u32)region->data_addr, region->block_sz);

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

		void write_rtt(u32 base, u32 size, u32 texaddr)
		{
			//Actually download the data, since it seems that cell is writing to it manually
			throw;
		}

		void destroy_rtt_cache()
		{
			for (cached_rtt &rtt : rtt_cache)
			{
				rtt.valid = false;
				rtt.is_dirty = false;
				rtt.block_sz = 0;
				rtt.data_addr = 0;

				glDeleteTextures(1, &rtt.copy_glid);
				rtt.copy_glid = 0;
			}

			rtt_cache.resize(0);
		}

	public:

		gl_texture_cache()
			: frame_ctr(0)
		{
		}
		
		~gl_texture_cache()
		{
			clear_obj_cache();
		}

		void update_frame_ctr()
		{
			frame_ctr++;
		}

		void initialize_rtt_cache()
		{
			if (rtt_cache.size()) throw EXCEPTION("Initialize RTT cache while cache already exists! Leaking objects??");

			for (int i = 0; i < 64; ++i)
			{
				cached_rtt rtt;

				glGenTextures(1, &rtt.copy_glid);
				rtt.is_dirty = true;
				rtt.valid = true;
				rtt.block_sz = 0;
				rtt.data_addr = 0;
				rtt.locked = false;

				rtt_cache.push_back(rtt);
			}
		}

		template<typename RsxTextureType>
		void upload_texture(int index, RsxTextureType &tex, rsx::gl::texture &gl_texture, gl_render_targets &m_rtts)
		{
			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
			const u32 range = (u32)get_texture_size(tex);

			glActiveTexture(GL_TEXTURE0 + index);

			/**
			 * Give precedence to rtt data obtained through read/write buffers
			 */
			cached_rtt *rtt = find_cached_rtt(texaddr, range);
			
			if (rtt && !rtt->is_dirty)
			{
				u32 real_id = gl_texture.id();

				gl_texture.set_id(rtt->copy_glid);
				gl_texture.bind();

				gl_texture.set_id(real_id);
			}

			/**
			 * Check for sampleable rtts from previous render passes
			 */
			gl::texture *texptr = nullptr;
			if (texptr = m_rtts.get_texture_from_render_target_if_applicable(texaddr))
			{
				texptr->bind();
				return;
			}

			if (texptr = m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr))
			{
				texptr->bind();
				return;
			}

			/**
			 * If all the above failed, then its probably a generic texture.
			 * Search in cache and upload/bind
			 */
			
			gl_cached_texture *obj = nullptr;

			if (!rtt)
				obj = find_obj_for_params(texaddr, tex.width(), tex.height(), tex.get_exact_mipmap_count());

			if (obj && !obj->deleted)
			{
				u32 real_id = gl_texture.id();

				gl_texture.set_id(obj->gl_id);
				gl_texture.bind();

				gl_texture.set_id(real_id);
			}
			else
			{
				u32 real_id = gl_texture.id();

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
				gl_cached_texture &_obj = create_obj_for_params(gl_texture.id(), texaddr, tex.width(), tex.height(), tex.get_exact_mipmap_count());

				_obj.block_sz = (u32)get_texture_size(tex);
				lock_gl_object(_obj);

				gl_texture.set_id(real_id);
			}
		}

		bool mark_as_dirty(u32 address)
		{
			if (address < texture_cache_range.first ||
				address > texture_cache_range.second)
				return false;

			bool response = false;

			for (gl_cached_texture &tex: texture_cache)
			{
				if (!tex.locked) continue;

				if (tex.protected_block_start <= address &&
					tex.protected_block_sz >(address - tex.protected_block_start))
				{
					unlock_gl_object(tex);

					invalidate_rtts_in_range((u32)tex.data_addr, tex.block_sz);

					tex.deleted = true;
					response = true;
				}
			}

			if (response) return true;

			for (cached_rtt &rtt: rtt_cache)
			{
				if (!rtt.data_addr || rtt.is_dirty) continue;

				u32 rtt_aligned_base = ((u32)(rtt.data_addr)) & ~(4096 - 1);
				u32 rtt_block_sz = align(rtt.block_sz, 4096);
				
				if (rtt.locked && (u64)address >= rtt_aligned_base)
				{
					u32 offset = address - rtt_aligned_base;
					if (offset >= rtt_block_sz) continue;

					rtt.is_dirty = true;

					unlock_memory_region(rtt_aligned_base, rtt_block_sz);
					rtt.locked = false;

					response = true;
				}
			}

			return response;
		}

		void save_render_target(u32 texaddr, u32 range, gl::texture &gl_texture)
		{
			save_rtt(texaddr, range, gl_texture.width(), gl_texture.height(), (GLenum)gl_texture.get_internal_format(), gl_texture);
		}

		std::vector<invalid_cache_area> find_and_invalidate_in_range(u32 base, u32 limit)
		{
			/**
			* Sometimes buffers can share physical pages.
			* Return objects if we really encroach on texture
			*/

			std::vector<invalid_cache_area> result;

			for (gl_cached_texture &obj : texture_cache)
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

		void lock_invalidated_ranges(std::vector<invalid_cache_area> invalid)
		{
			for (invalid_cache_area area : invalid)
			{
				lock_memory_region(area.block_base, area.block_sz);
			}
		}

		void remove_in_range(u32 texaddr, u32 range)
		{
			//Seems that the rsx only 'reads' full texture objects..
			//This simplifies this function to simply check for matches
			for (gl_cached_texture &cached : texture_cache)
			{
				if (cached.data_addr == texaddr &&
					cached.block_sz == range)
					remove_obj(cached);
			}
		}

		bool explicit_writeback(gl::texture &tex, const u32 address, const u32 pitch)
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
	};
}