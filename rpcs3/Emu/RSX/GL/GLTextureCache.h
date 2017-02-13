#pragma once

#include "stdafx.h"

#include <exception>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>

#include "GLGSRender.h"
#include "GLRenderTargets.h"
#include "../Common/TextureUtils.h"
#include <chrono>

namespace rsx
{
	//TODO: Properly move this into rsx shared
	class buffered_section
	{
	protected:
		u32 cpu_address_base = 0;
		u32 cpu_address_range = 0;

		u32 locked_address_base = 0;
		u32 locked_address_range = 0;

		u32 memory_protection = 0;

		bool locked = false;
		bool dirty = false;

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

	public:

		buffered_section() {}
		~buffered_section() {}

		void reset(u32 base, u32 length)
		{
			verify(HERE), locked == false;

			cpu_address_base = base;
			cpu_address_range = length;

			locked_address_base = (base & ~4095);
			locked_address_range = align(base + length, 4096) - locked_address_base;

			memory_protection = vm::page_readable|vm::page_writable;

			locked = false;
		}

		bool protect(u8 flags_set, u8 flags_clear)
		{
			if (vm::page_protect(locked_address_base, locked_address_range, 0, flags_set, flags_clear))
			{
				memory_protection &= ~flags_clear;
				memory_protection |= flags_set;

				locked = memory_protection != (vm::page_readable | vm::page_writable);
			}
			else
				fmt::throw_exception("failed to lock memory @ 0x%X!", locked_address_base);

			return false;
		}

		bool unprotect()
		{
			u32 flags_set = (vm::page_readable | vm::page_writable) & ~memory_protection;

			if (vm::page_protect(locked_address_base, locked_address_range, 0, flags_set, 0))
			{
				memory_protection = (vm::page_writable | vm::page_readable);
				locked = false;
				return true;
			}
			else
				fmt::throw_exception("failed to unlock memory @ 0x%X!", locked_address_base);

			return false;
		}

		bool overlaps(std::pair<u32, u32> range)
		{
			return region_overlaps(locked_address_base, locked_address_base+locked_address_range, range.first, range.first + range.second);
		}

		bool overlaps(u32 address)
		{
			return (locked_address_base <= address && (address - locked_address_base) < locked_address_range);
		}

		bool is_locked() const
		{
			return locked;
		}

		bool is_dirty() const
		{
			return dirty;
		}

		void set_dirty(bool state)
		{
			dirty = state;
		}

		u32 get_section_base() const
		{
			return cpu_address_base;
		}

		u32 get_section_size() const
		{
			return cpu_address_range;
		}

		bool matches(u32 cpu_address, u32 size) const
		{
			return (cpu_address_base == cpu_address && cpu_address_range == size);
		}

		std::pair<u32, u32> get_min_max(std::pair<u32, u32> current_min_max)
		{
			u32 min = std::min(current_min_max.first, locked_address_base);
			u32 max = std::max(current_min_max.second, locked_address_base + locked_address_range);

			return std::make_pair(min, max);
		}
	};
}

namespace gl
{
	//TODO: Properly move this into helpers
	class fence
	{
		GLsync m_value = nullptr;
		GLenum flags = GL_SYNC_FLUSH_COMMANDS_BIT;

	public:

		fence() {}
		~fence() {}

		void create()
		{
			m_value = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		}

		void destroy()
		{
			glDeleteSync(m_value);
			m_value = nullptr;
		}

		void reset()
		{
			if (m_value != nullptr)
				destroy();

			create();
		}

		bool check_signaled()
		{
			verify(HERE), m_value != nullptr;

			GLenum err = glClientWaitSync(m_value, flags, 0);
			flags = 0;
			return (err == GL_ALREADY_SIGNALED || err == GL_CONDITION_SATISFIED);
		}

		bool wait_for_signal()
		{
			verify(HERE), m_value != nullptr;

			GLenum err = GL_WAIT_FAILED;
			bool done = false;
			
			while (!done)
			{
				//Check if we are finished, wait time = 1us
				err = glClientWaitSync(m_value, flags, 1000);
				flags = 0;

				switch (err)
				{
				default:
					LOG_ERROR(RSX, "gl::fence sync returned unknown error 0x%X", err);
				case GL_ALREADY_SIGNALED:
				case GL_CONDITION_SATISFIED:
					done = true;
					break;
				case GL_TIMEOUT_EXPIRED:
					continue;
				}
			}

			glDeleteSync(m_value);
			m_value = nullptr;

			return (err == GL_ALREADY_SIGNALED || err == GL_CONDITION_SATISFIED);
		}
	};


	//TODO: Unify all cache objects
	class texture_cache
	{
	public:

		class cached_texture_section : public rsx::buffered_section
		{
			u32 texture_id = 0;
			u32 width = 0;
			u32 height = 0;
			u16 mipmaps = 0;

		public:

			void create(u32 id, u32 width, u32 height, u32 mipmaps)
			{
				verify(HERE), locked == false;

				texture_id = id;
				this->width = width;
				this->height = height;
				this->mipmaps = mipmaps;
			}

			bool matches(u32 rsx_address, u32 width, u32 height, u32 mipmaps)
			{
				if (rsx_address == cpu_address_base && texture_id != 0)
				{
					if (!width && !height && !mipmaps)
						return true;

					return (width == this->width && height == this->height && mipmaps == this->mipmaps);
				}

				return false;
			}

			void destroy()
			{
				if (locked)
					unprotect();

				glDeleteTextures(1, &texture_id);
				texture_id = 0;
			}

			bool is_empty()
			{
				return (texture_id == 0);
			}

			u32 id() const
			{
				return texture_id;
			}
		};

		class cached_rtt_section : public rsx::buffered_section
		{
		private:
			fence m_fence;
			u32 pbo_id = 0;
			u32 pbo_size = 0;

			bool flushed = false;
			bool is_depth = false;

			u32 flush_count = 0;
			u32 copy_count = 0;

			u32 current_width = 0;
			u32 current_height = 0;
			u32 current_pitch = 0;
			u32 real_pitch = 0;

			texture::format format = texture::format::rgba;
			texture::type type = texture::type::ubyte;

			u8 get_pixel_size(texture::format fmt_, texture::type type_)
			{
				u8 size = 1;
				switch (type_)
				{
				case texture::type::ubyte:
				case texture::type::sbyte:
					break;
				case texture::type::ushort:
				case texture::type::sshort:
				case texture::type::f16:
					size = 2;
					break;
				case texture::type::ushort_5_6_5:
				case texture::type::ushort_5_6_5_rev:
				case texture::type::ushort_4_4_4_4:
				case texture::type::ushort_4_4_4_4_rev:
				case texture::type::ushort_5_5_5_1:
				case texture::type::ushort_1_5_5_5_rev:
					return 2;
				case texture::type::uint_8_8_8_8:
				case texture::type::uint_8_8_8_8_rev:
				case texture::type::uint_10_10_10_2:
				case texture::type::uint_2_10_10_10_rev:
				case texture::type::uint_24_8:
					return 4;
				case texture::type::f32:
				case texture::type::sint:
				case texture::type::uint:
					size = 4;
					break;
				}

				switch (fmt_)
				{
				case texture::format::red:
				case texture::format::r:
					break;
				case texture::format::rg:
					size *= 2;
					break;
				case texture::format::rgb:
				case texture::format::bgr:
					size *= 3;
					break;
				case texture::format::rgba:
				case texture::format::bgra:
					size *= 4;
					break;

				//Depth formats..
				case texture::format::depth:
					size = 2;
					break;
				case texture::format::depth_stencil:
					size = 4;
					break;
				default:
					LOG_ERROR(RSX, "Unsupported rtt format %d", (GLenum)fmt_);
					size = 4;
				}

				return size;
			}

		public:

			void reset(u32 base, u32 size)
			{
				rsx::buffered_section::reset(base, size);
				flushed = false;
				flush_count = 0;
				copy_count = 0;
			}

			void init_buffer()
			{
				glGenBuffers(1, &pbo_id);

				glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_id);
				glBufferStorage(GL_PIXEL_PACK_BUFFER, locked_address_range, nullptr, GL_MAP_READ_BIT);

				pbo_size = locked_address_range;
			}

			void set_dimensions(u32 width, u32 height, u32 pitch)
			{
				current_width = width;
				current_height = height;
				current_pitch = pitch;

				real_pitch = width * get_pixel_size(format, type);
			}

			void set_format(texture::format gl_format, texture::type gl_type)
			{
				format = gl_format;
				type = gl_type;

				real_pitch = current_width * get_pixel_size(format, type);
			}

			void copy_texture(gl::texture &source)
			{
				glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_id);
				glGetTextureImage(source.id(), 0, (GLenum)format, (GLenum)type, pbo_size, nullptr);
				glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

				m_fence.reset();
				copy_count++;
			}

			void fill_texture(gl::texture &tex)
			{
				u32 min_width = std::min((u32)tex.width(), current_width);
				u32 min_height = std::min((u32)tex.height(), current_height);

				tex.bind();
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_id);
				glTexSubImage2D((GLenum)tex.get_target(), 0, 0, 0, min_width, min_height, (GLenum)format, (GLenum)type, nullptr);
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			}

			template <typename T, int N>
			void scale_image_impl(T* dst, const T* src, u16 src_width, u16 src_height, u16 padding)
			{
				u32 dst_offset = 0;
				u32 src_offset = 0;

				for (u16 h = 0; h < src_height; ++h)
				{
					for (u16 w = 0; w < src_width; ++w)
					{
						for (u8 n = 0; n < N; ++n)
						{
							dst[dst_offset++] = src[src_offset];
						}

						//Fetch next pixel
						src_offset++;
					}

					//Pad this row
					dst_offset += padding;
				}
			}

			template <int N>
			void scale_image(void *dst, void *src, u8 pixel_size, u16 src_width, u16 src_height, u16 padding)
			{
				switch (pixel_size)
				{
				case 1:
					scale_image_impl<u8, N>((u8*)dst, (u8*)src, current_width, current_height, padding);
					break;
				case 2:
					scale_image_impl<u16, N>((u16*)dst, (u16*)src, current_width, current_height, padding);
					break;
				case 4:
					scale_image_impl<u32, N>((u32*)dst, (u32*)src, current_width, current_height, padding);
					break;
				case 8:
					scale_image_impl<u64, N>((u64*)dst, (u64*)src, current_width, current_height, padding);
					break;
				default:
					fmt::throw_exception("unsupported rtt format 0x%X" HERE, (u32)format);
				}
			}

			void flush()
			{
				protect(vm::page_writable, 0);
				m_fence.wait_for_signal();
				flushed = true;

				glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_id);
				void *data = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, pbo_size, GL_MAP_READ_BIT);
				u8 *dst = vm::ps3::_ptr<u8>(cpu_address_base);

				//throw if map failed since we'll segfault anyway
				verify(HERE), data != nullptr;

				if (real_pitch >= current_pitch)
					memcpy(dst, data, cpu_address_range);
				else
				{
					//Scale this image by repeating pixel data n times
					//n = expected_pitch / real_pitch
					//Use of fixed argument templates for performance reasons

					const u16 pixel_size = get_pixel_size(format, type);
					const u16 dst_width  = current_pitch / pixel_size;
					const u16 sample_count = current_pitch / real_pitch;
					const u16 padding = dst_width - (current_width * sample_count);

					switch (sample_count)
					{
					case 2:
						scale_image<2>(dst, data, pixel_size, current_width, current_height, padding);
						break;
					case 3:
						scale_image<3>(dst, data, pixel_size, current_width, current_height, padding);
						break;
					case 4:
						scale_image<4>(dst, data, pixel_size, current_width, current_height, padding);
						break;
					case 8:
						scale_image<8>(dst, data, pixel_size, current_width, current_height, padding);
						break;
					case 16:
						scale_image<16>(dst, data, pixel_size, current_width, current_height, padding);
						break;
					default:
						LOG_ERROR(RSX, "Unsupported RTT scaling factor: dst_pitch=%d src_pitch=%d", current_pitch, real_pitch);
						memcpy(dst, data, cpu_address_range);
					}
				}

				glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
				glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
				protect(vm::page_readable, vm::page_writable);

				flush_count++;
			}

			void destroy()
			{
				if (locked)
					unprotect();

				glDeleteBuffers(1, &pbo_id);
				pbo_id = 0;
				pbo_size = 0;

				m_fence.destroy();
			}

			bool is_flushed() const
			{
				return flushed;
			}

			bool can_skip()
			{
				//TODO: Better balancing algorithm. Copying buffers is very expensive
				//TODO: Add a switch to force strict enforcement

				//Always accept the first attempt at caching after creation
				if (!copy_count)
					return false;

				//If surface is flushed often, force buffering
				if (flush_count)
				{
					//TODO: Pick better values. Using 80% and 20% for now
					if (flush_count >= (4 * copy_count / 5))
						return false;
					else
					{
						if (flushed) return false;	//fence is guaranteed to have been signaled and destroyed
						return !m_fence.check_signaled();
					}
				}

				return true;
			}

			void set_flushed(bool state)
			{
				flushed = state;
			}
		};

	private:
		std::vector<cached_texture_section> m_texture_cache;
		std::vector<cached_rtt_section> m_rtt_cache;

		std::pair<u32, u32> texture_cache_range = std::make_pair(0xFFFFFFFF, 0);
		std::pair<u32, u32> rtt_cache_range = std::make_pair(0xFFFFFFFF, 0);

		std::mutex m_section_mutex;

		cached_texture_section *find_texture(u64 texaddr, u32 w, u32 h, u16 mipmaps)
		{
			for (cached_texture_section &tex : m_texture_cache)
			{
				if (tex.matches(texaddr, w, h, mipmaps) && !tex.is_dirty())
					return &tex;
			}

			return nullptr;
		}

		cached_texture_section& create_texture(u32 id, u32 texaddr, u32 texsize, u32 w, u32 h, u16 mipmap)
		{
			for (cached_texture_section &tex : m_texture_cache)
			{
				if (tex.is_dirty())
				{
					tex.destroy();
					tex.reset(texaddr, texsize);
					tex.create(id, w, h, mipmap);
					
					texture_cache_range = tex.get_min_max(texture_cache_range);
					return tex;
				}
			}

			cached_texture_section tex;
			tex.reset(texaddr, texsize);
			tex.create(id, w, h, mipmap);
			texture_cache_range = tex.get_min_max(texture_cache_range);

			m_texture_cache.push_back(tex);
			return m_texture_cache.back();
		}

		void clear()
		{
			for (cached_texture_section &tex : m_texture_cache)
			{
				tex.destroy();
			}

			for (cached_rtt_section &rtt : m_rtt_cache)
			{
				rtt.destroy();
			}

			m_rtt_cache.resize(0);
			m_texture_cache.resize(0);
		}

		cached_rtt_section* find_cached_rtt_section(u32 base, u32 size)
		{
			for (cached_rtt_section &rtt : m_rtt_cache)
			{
				if (rtt.matches(base, size))
				{
					return &rtt;
				}
			}

			return nullptr;
		}

		cached_rtt_section *create_locked_view_of_section(u32 base, u32 size)
		{
			cached_rtt_section *region = find_cached_rtt_section(base, size);

			if (!region)
			{
				for (cached_rtt_section &rtt : m_rtt_cache)
				{
					if (rtt.is_dirty())
					{
						rtt.reset(base, size);
						rtt.protect(0, vm::page_readable | vm::page_writable);
						region = &rtt;
						break;
					}
				}

				if (!region)
				{
					cached_rtt_section section;
					section.reset(base, size);
					section.set_dirty(true);
					section.init_buffer();
					section.protect(0, vm::page_readable | vm::page_writable);

					m_rtt_cache.push_back(section);
					region = &m_rtt_cache.back();
				}

				rtt_cache_range = region->get_min_max(rtt_cache_range);
			}
			else
			{
				//This section view already exists
				if (region->get_section_size() != size)
				{
					region->unprotect();
					region->reset(base, size);
				}

				if (!region->is_locked() || region->is_flushed())
					region->protect(0, vm::page_readable | vm::page_writable);
			}

			return region;
		}

	public:

		texture_cache() {}

		~texture_cache()
		{
			clear();
		}

		template<typename RsxTextureType>
		void upload_texture(int index, RsxTextureType &tex, rsx::gl::texture &gl_texture, gl_render_targets &m_rtts)
		{
			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
			const u32 range = (u32)get_texture_size(tex);

			glActiveTexture(GL_TEXTURE0 + index);

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

			cached_texture_section *cached_texture = find_texture(texaddr, tex.width(), tex.height(), tex.get_exact_mipmap_count());
			verify(HERE), gl_texture.id() == 0;

			if (cached_texture)
			{
				verify(HERE), cached_texture->is_empty() == false;

				gl_texture.set_id(cached_texture->id());
				gl_texture.bind();

				//external gl::texture objects should always be undefined/uninitialized!
				gl_texture.set_id(0);
				return;
			}

			if (!tex.width() || !tex.height())
			{
				LOG_ERROR(RSX, "Texture upload requested but invalid texture dimensions passed");
				return;
			}

			gl_texture.init(index, tex);

			std::lock_guard<std::mutex> lock(m_section_mutex);

			cached_texture_section &cached = create_texture(gl_texture.id(), texaddr, get_texture_size(tex), tex.width(), tex.height(), tex.get_exact_mipmap_count());
			cached.protect(0, vm::page_writable);
			cached.set_dirty(false);

			//external gl::texture objects should always be undefined/uninitialized!
			gl_texture.set_id(0);
		}

		void save_rtt(u32 base, u32 size, gl::texture &source, u32 width, u32 height, u32 pitch, texture::format format, texture::type type)
		{
			std::lock_guard<std::mutex> lock(m_section_mutex);

			cached_rtt_section *region = create_locked_view_of_section(base, size);

			//Ignore this if we haven't finished downloading previous draw call
			//TODO: Separate locking sections vs downloading to pbo unless address faults often
			if (0)//region->can_skip())
				return;

			if (!region->matches(base, size))
			{
				//This memory region overlaps our own region, but does not match it exactly
				if (region->is_locked())
					region->unprotect();

				region->reset(base, size);
				region->protect(0, vm::page_readable | vm::page_writable);
			}

			region->set_dimensions(width, height, pitch);
			region->copy_texture(source);
			region->set_format(format, type);
			region->set_dirty(false);
			region->set_flushed(false);

			verify(HERE), region->is_locked() == true;
		}

		bool load_rtt(gl::texture &tex, const u32 address, const u32 pitch)
		{
			const u32 range = tex.height() * pitch;
			cached_rtt_section *rtt = find_cached_rtt_section(address, range);

			if (rtt && !rtt->is_dirty())
			{
				rtt->fill_texture(tex);
				return true;
			}

			//No valid object found in cache
			return false;
		}

		bool mark_as_dirty(u32 address)
		{
			bool response = false;

			if (address >= texture_cache_range.first &&
				address < texture_cache_range.second)
			{
				std::lock_guard<std::mutex> lock(m_section_mutex);

				for (cached_texture_section &tex : m_texture_cache)
				{
					if (!tex.is_locked()) continue;

					if (tex.overlaps(address))
					{
						tex.unprotect();
						tex.set_dirty(true);

						response = true;
					}
				}
			}

			if (address >= rtt_cache_range.first &&
				address < rtt_cache_range.second)
			{
				std::lock_guard<std::mutex> lock(m_section_mutex);

				for (cached_rtt_section &rtt : m_rtt_cache)
				{
					if (rtt.is_dirty()) continue;

					if (rtt.is_locked() && rtt.overlaps(address))
					{
						rtt.unprotect();
						rtt.set_dirty(true);

						response = true;
					}
				}
			}

			return response;
		}

		void invalidate_range(u32 base, u32 size)
		{
			std::lock_guard<std::mutex> lock(m_section_mutex);
			std::pair<u32, u32> range = std::make_pair(base, size);

			if (base < texture_cache_range.second &&
				(base + size) >= texture_cache_range.first)
			{
				for (cached_texture_section &tex : m_texture_cache)
				{
					if (!tex.is_dirty() && tex.overlaps(range))
						tex.destroy();
				}
			}

			if (base < rtt_cache_range.second &&
				(base + size) >= rtt_cache_range.first)
			{
				for (cached_rtt_section &rtt : m_rtt_cache)
				{
					if (!rtt.is_dirty() && rtt.overlaps(range))
					{
						rtt.unprotect();
						rtt.set_dirty(true);
					}
				}
			}
		}

		bool flush_section(u32 address)
		{
			if (address < rtt_cache_range.first ||
				address >= rtt_cache_range.second)
				return false;

			std::lock_guard<std::mutex> lock(m_section_mutex);

			for (cached_rtt_section &rtt : m_rtt_cache)
			{
				if (rtt.is_dirty()) continue;

				if (rtt.is_locked() && rtt.overlaps(address))
				{
					if (rtt.is_flushed())
					{
						LOG_WARNING(RSX, "Section matches range, but marked as already flushed!, 0x%X+0x%X", rtt.get_section_base(), rtt.get_section_size());
						continue;
					}

					rtt.flush();
					return true;
				}
			}

			return false;
		}
	};
}