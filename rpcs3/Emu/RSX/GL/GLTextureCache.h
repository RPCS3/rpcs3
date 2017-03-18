#pragma once

#include "stdafx.h"

#include <exception>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <thread>
#include <condition_variable>
#include <chrono>

#include "GLRenderTargets.h"
#include "../Common/TextureUtils.h"
#include "../../Memory/vm.h"
#include "Utilities/Config.h"

class GLGSRender;

extern cfg::bool_entry g_cfg_rsx_write_color_buffers;
extern cfg::bool_entry g_cfg_rsx_write_depth_buffer;

namespace gl
{
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

			bool matches(u32 rsx_address, u32 width, u32 height, u32 mipmaps) const
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

			bool is_empty() const
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

			u32 source_texture = 0;

			bool copied = false;
			bool flushed = false;
			bool is_depth = false;

			u32 current_width = 0;
			u32 current_height = 0;
			u32 current_pitch = 0;
			u32 real_pitch = 0;

			texture::format format = texture::format::rgba;
			texture::type type = texture::type::ubyte;
			bool pack_unpack_swap_bytes = false;

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

			void scale_image_fallback(u8* dst, const u8* src, u16 src_width, u16 src_height, u16 dst_pitch, u16 src_pitch, u8 pixel_size, u8 samples)
			{
				u32 dst_offset = 0;
				u32 src_offset = 0;
				u32 padding = dst_pitch - (src_pitch * samples);

				for (u16 h = 0; h < src_height; ++h)
				{
					for (u16 w = 0; w < src_width; ++w)
					{
						for (u8 n = 0; n < samples; ++n)
						{
							memcpy(&dst[dst_offset], &src[src_offset], pixel_size);
							dst_offset += pixel_size;
						}

						src_offset += pixel_size;
					}

					dst_offset += padding;
				}
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

			void init_buffer()
			{
				if (pbo_id)
				{
					glDeleteBuffers(1, &pbo_id);
					pbo_id = 0;
					pbo_size = 0;
				}

				glGenBuffers(1, &pbo_id);

				glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_id);
				glBufferStorage(GL_PIXEL_PACK_BUFFER, locked_address_range, nullptr, GL_MAP_READ_BIT);

				pbo_size = locked_address_range;
			}

		public:

			void reset(u32 base, u32 size)
			{
				rsx::buffered_section::reset(base, size);
				init_buffer();
				
				flushed = false;
				copied = false;

				source_texture = 0;
			}

			void set_dimensions(u32 width, u32 height, u32 pitch)
			{
				current_width = width;
				current_height = height;
				current_pitch = pitch;

				real_pitch = width * get_pixel_size(format, type);
			}

			void set_format(texture::format gl_format, texture::type gl_type, bool swap_bytes)
			{
				format = gl_format;
				type = gl_type;
				pack_unpack_swap_bytes = swap_bytes;

				real_pitch = current_width * get_pixel_size(format, type);
			}

			void set_source(gl::texture &source)
			{
				source_texture = source.id();
			}

			void copy_texture()
			{
				if (!glIsTexture(source_texture))
				{
					LOG_ERROR(RSX, "Attempted to download rtt texture, but texture handle was invalid! (0x%X)", source_texture);
					return;
				}

				glPixelStorei(GL_PACK_SWAP_BYTES, pack_unpack_swap_bytes);
				glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_id);
				glGetTextureImageEXT(source_texture, GL_TEXTURE_2D, 0, (GLenum)format, (GLenum)type, nullptr);
				glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

				m_fence.reset();
				copied = true;
			}

			void fill_texture(gl::texture &tex)
			{
				if (!copied)
				{
					//LOG_WARNING(RSX, "Request to fill texture rejected because contents were not read");
					return;
				}

				u32 min_width = std::min((u32)tex.width(), current_width);
				u32 min_height = std::min((u32)tex.height(), current_height);

				tex.bind();
				glPixelStorei(GL_UNPACK_SWAP_BYTES, pack_unpack_swap_bytes);
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_id);
				glTexSubImage2D((GLenum)tex.get_target(), 0, 0, 0, min_width, min_height, (GLenum)format, (GLenum)type, nullptr);
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			}

			void flush()
			{
				if (!copied)
				{
					LOG_WARNING(RSX, "Cache miss at address 0x%X. This is gonna hurt...", cpu_address_base);
					copy_texture();

					if (!copied)
					{
						LOG_WARNING(RSX, "Nothing to copy; Setting section to readable and moving on...");
						protect(utils::protection::ro);
						return;
					}
				}

				protect(utils::protection::rw);
				m_fence.wait_for_signal();
				flushed = true;

				glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_id);
				void *data = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, pbo_size, GL_MAP_READ_BIT);
				u8 *dst = vm::ps3::_ptr<u8>(cpu_address_base);

				//throw if map failed since we'll segfault anyway
				verify(HERE), data != nullptr;

				if (real_pitch >= current_pitch)
				{
					memcpy(dst, data, cpu_address_range);
				}
				else
				{
					//TODO: Use compression hint from the gcm tile information
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
						scale_image_fallback(dst, static_cast<u8*>(data), current_width, current_height, current_pitch, real_pitch, pixel_size, sample_count);
					}
				}

				glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
				glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
				protect(utils::protection::ro);
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

			void set_flushed(bool state)
			{
				flushed = state;
			}

			void set_copied(bool state)
			{
				copied = state;
			}
		};

		class blitter
		{
			fbo fbo_argb8;
			fbo fbo_rgb565;
			fbo blit_src;

			u32 argb8_surface = 0;
			u32 rgb565_surface = 0;

		public:

			void init()
			{
				fbo_argb8.create();
				fbo_rgb565.create();
				blit_src.create();

				glGenTextures(1, &argb8_surface);
				glBindTexture(GL_TEXTURE_2D, argb8_surface);
				glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 4096, 4096);

				glGenTextures(1, &rgb565_surface);
				glBindTexture(GL_TEXTURE_2D, rgb565_surface);
				glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB565, 4096, 4096);

				s32 old_fbo = 0;
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);

				fbo_argb8.bind();
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, argb8_surface, 0);
				
				fbo_rgb565.bind();
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rgb565_surface, 0);

				glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);

				fbo_argb8.check();
				fbo_rgb565.check();
			}

			void destroy()
			{
				fbo_argb8.remove();
				fbo_rgb565.remove();
				blit_src.remove();

				glDeleteTextures(1, &argb8_surface);
				glDeleteTextures(1, &rgb565_surface);
			}

			u32 scale_image(u32 src, u32 dst, const areai src_rect, const areai dst_rect, const position2i dst_offset, const position2i clip_offset,
					const size2i dst_dims, const size2i clip_dims, bool is_argb8, bool linear_interpolation)
			{
				s32 old_fbo = 0;
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);
				
				blit_src.bind();
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, src, 0);
				blit_src.check();

				u32 src_surface = 0;
				u32 dst_tex = dst;
				filter interp = linear_interpolation ? filter::linear : filter::nearest;

				if (!dst_tex)
				{
					glGenTextures(1, &dst_tex);
					glBindTexture(GL_TEXTURE_2D, dst_tex);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

					if (is_argb8)
						glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, dst_dims.width, dst_dims.height);
					else
						glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB565, dst_dims.width, dst_dims.height);
				}

				if (is_argb8)
				{
					blit_src.blit(fbo_argb8, src_rect, dst_rect, buffers::color, interp);
					src_surface = argb8_surface;
				}
				else
				{
					blit_src.blit(fbo_rgb565, src_rect, dst_rect, buffers::color, interp);
					src_surface = rgb565_surface;
				}

				glCopyImageSubData(src_surface, GL_TEXTURE_2D, 0, clip_offset.x, clip_offset.y, 0,
					dst_tex, GL_TEXTURE_2D, 0, dst_offset.x, dst_offset.y, 0, clip_dims.width, clip_dims.height, 1);

				glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
				return dst_tex;
			}
		};

	private:
		std::vector<cached_texture_section> m_texture_cache;
		std::vector<cached_rtt_section> m_rtt_cache;
		std::vector<u32> m_temporary_surfaces;

		std::pair<u32, u32> texture_cache_range = std::make_pair(0xFFFFFFFF, 0);
		std::pair<u32, u32> rtt_cache_range = std::make_pair(0xFFFFFFFF, 0);

		blitter m_hw_blitter;

		std::mutex m_section_mutex;

		GLGSRender *m_renderer;
		std::thread::id m_renderer_thread;

		cached_texture_section *find_texture(u64 texaddr, u32 w, u32 h, u16 mipmaps)
		{
			for (cached_texture_section &tex : m_texture_cache)
			{
				if (tex.matches(texaddr, w, h, mipmaps) && !tex.is_dirty())
					return &tex;
			}

			return nullptr;
		}

		cached_texture_section *find_texture(u32 texaddr, u32 range)
		{
			auto test = std::make_pair(texaddr, range);
			for (cached_texture_section &tex : m_texture_cache)
			{
				if (tex.overlaps(test, true) && !tex.is_dirty())
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

			clear_temporary_surfaces();
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
						rtt.protect(utils::protection::no);
						region = &rtt;
						break;
					}
				}

				if (!region)
				{
					cached_rtt_section section;
					section.reset(base, size);
					section.set_dirty(true);
					section.protect(utils::protection::no);

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
				{
					region->protect(utils::protection::no);
				}
			}

			return region;
		}

		u32 create_temporary_subresource(u32 src_id, GLenum sized_internal_fmt, u16 x, u16 y, u16 width, u16 height)
		{
			u32 dst_id = 0;

			glGenTextures(1, &dst_id);
			glBindTexture(GL_TEXTURE_2D, dst_id);

			glTexStorage2D(GL_TEXTURE_2D, 1, sized_internal_fmt, width, height);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			//Empty GL_ERROR
			glGetError();

			glCopyImageSubData(src_id, GL_TEXTURE_2D, 0, x, y, 0,
				dst_id, GL_TEXTURE_2D, 0, 0, 0, 0, width, height, 1);

			m_temporary_surfaces.push_back(dst_id);

			//Check for error
			if (GLenum err = glGetError())
			{
				LOG_WARNING(RSX, "Failed to copy image subresource with GL error 0x%X", err);
				return 0;
			}
			
			return dst_id;
		}

	public:

		texture_cache() {}

		~texture_cache() {}

		void initialize(GLGSRender *renderer)
		{
			m_renderer = renderer;
			m_renderer_thread = std::this_thread::get_id();

			m_hw_blitter.init();
		}

		void close()
		{
			clear();

			m_hw_blitter.destroy();
		}

		template<typename RsxTextureType>
		void upload_texture(int index, RsxTextureType &tex, rsx::gl::texture &gl_texture, gl_render_targets &m_rtts)
		{
			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
			const u32 range = (u32)get_texture_size(tex);

			if (!texaddr || !range)
			{
				LOG_ERROR(RSX, "Texture upload requested but texture not found, (address=0x%X, size=0x%X)", texaddr, range);
				gl_texture.bind();
				return;
			}

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

			//LOG_ERROR(RSX, "REGULAR IFC: address=0x%X + 0x%X, w=%d h=%d", texaddr, range, tex.width(), tex.height());

			/**
			 * Check if we are re-sampling a subresource of an RTV/DSV texture, bound or otherwise
			 * (Turbo: Super Stunt Squad does this; bypassing the need for a sync object)
			 * The engine does not read back the texture resource through cell, but specifies a texture location that is
			 * a bound render target. We can bypass the expensive download in this case
			 */

			const u32 format = tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
			const f32 internal_scale = (f32)tex.pitch() / (tex.width() * get_format_block_size_in_bytes(format));
			const u32 internal_width = tex.width() * internal_scale;

			const surface_subresource rsc = m_rtts.get_surface_subresource_if_applicable(texaddr, internal_width, tex.height(), tex.pitch(), true);
			if (rsc.surface)
			{
				//Check that this region is not cpu-dirty before doing a copy
				//This section is guaranteed to have a locking section *if* this bit has been bypassed before

				bool upload_from_cpu = false;

				for (cached_rtt_section &section : m_rtt_cache)
				{
					if (section.overlaps(std::make_pair(texaddr, range)) && section.is_dirty())
					{
						LOG_ERROR(RSX, "Cell wrote to render target section we are uploading from!");

						upload_from_cpu = true;
						break;
					}
				}

				if (!upload_from_cpu)
				{
					if (tex.get_extended_texture_dimension() != rsx::texture_dimension_extended::texture_dimension_2d)
					{
						LOG_ERROR(RSX, "Sampling of RTT region as non-2D texture! addr=0x%x, Type=%d, dims=%dx%d",
								texaddr, (u8)tex.get_extended_texture_dimension(), tex.width(), tex.height());
					}
					else
					{
						GLenum src_format = (GLenum)rsc.surface->get_internal_format();
						GLenum dst_format = std::get<0>(get_format_type(format));

						u32 bound_index = ~0U;

						if (src_format != dst_format)
						{
							LOG_WARNING(RSX, "Sampling from a section of a render target, but formats might be incompatible (0x%X vs 0x%X)", src_format, dst_format);
						}

						if (!rsc.is_bound)
						{
							if (rsc.w == tex.width() && rsc.h == tex.height())
								rsc.surface->bind();
							else
								bound_index = create_temporary_subresource(rsc.surface->id(), (GLenum)rsc.surface->get_compatible_internal_format(), rsc.x, rsc.y, rsc.w, rsc.h);
						}
						else
						{
							LOG_WARNING(RSX, "Attempting to sample a currently bound render target @ 0x%x", texaddr);
							bound_index = create_temporary_subresource(rsc.surface->id(), (GLenum)rsc.surface->get_compatible_internal_format(), rsc.x, rsc.y, rsc.w, rsc.h);
						}

						if (bound_index)
							return;
					}
				}
			}

			/**
			 * If all the above failed, then its probably a generic texture.
			 * Search in cache and upload/bind
			 */

			cached_texture_section *cached_texture = find_texture(texaddr, tex.width(), tex.height(), tex.get_exact_mipmap_count());

			if (cached_texture)
			{
				verify(HERE), cached_texture->is_empty() == false;

				gl_texture.set_id(cached_texture->id());
				gl_texture.bind();

				//external gl::texture objects should always be undefined/uninitialized!
				gl_texture.set_id(0);
				return;
			}

			/**
			 * Check for subslices from the cache in case we only have a subset a larger texture
			 */
			cached_texture = find_texture(texaddr, range);
			if (cached_texture)
			{
				if (texaddr >= cached_texture->get_section_base())
				{
					const u32 address_offset = texaddr - cached_texture->get_section_base();
					const u32 format = tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
					const GLenum ifmt = gl::get_sized_internal_format(format);

					u16 offset_x = 0, offset_y = 0;

					if (address_offset)
					{
						const u32 bpp = get_format_block_size_in_bytes(format);

						offset_y = address_offset / tex.pitch();
						offset_x = address_offset % tex.pitch();

						offset_x /= bpp;
						offset_y /= bpp;
					}

					u32 texture_id = create_temporary_subresource(cached_texture->id(), ifmt, offset_x, offset_y, tex.width(), tex.height());
					if (texture_id) return;

				}
				else
					LOG_ERROR(RSX, "Broken cache overlap search");
			}

			gl_texture.init(index, tex);

			std::lock_guard<std::mutex> lock(m_section_mutex);

			cached_texture_section &cached = create_texture(gl_texture.id(), texaddr, get_texture_size(tex), tex.width(), tex.height(), tex.get_exact_mipmap_count());
			cached.protect(utils::protection::ro);
			cached.set_dirty(false);

			//external gl::texture objects should always be undefined/uninitialized!
			gl_texture.set_id(0);
		}

		void save_rtt(u32 base, u32 size)
		{
			std::lock_guard<std::mutex> lock(m_section_mutex);

			cached_rtt_section *region = find_cached_rtt_section(base, size);

			if (!region)
			{
				LOG_ERROR(RSX, "Attempted to download render target that does not exist. Please report to developers");
				return;
			}

			if (!region->is_locked())
			{
				verify(HERE), region->is_dirty();
				LOG_WARNING(RSX, "Cell write to bound render target area");

				region->protect(utils::protection::no);
				region->set_dirty(false);
			}

			region->copy_texture();
		}

		void lock_rtt_region(const u32 base, const u32 size, const u16 width, const u16 height, const u16 pitch, const texture::format format, const texture::type type, const bool swap_bytes, gl::texture &source)
		{
			std::lock_guard<std::mutex> lock(m_section_mutex);

			cached_rtt_section *region = create_locked_view_of_section(base, size);

			if (!region->matches(base, size))
			{
				//This memory region overlaps our own region, but does not match it exactly
				if (region->is_locked())
					region->unprotect();

				region->reset(base, size);
				region->protect(utils::protection::no);
			}

			region->set_dimensions(width, height, pitch);
			region->set_format(format, type, swap_bytes);
			region->set_dirty(false);
			region->set_flushed(false);
			region->set_copied(false);
			region->set_source(source);

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
			std::pair<u32, u32> trampled_range = std::make_pair(0xffffffff, 0x0);

			//TODO: Optimize this function!
			//Multi-pass checking is slow. Pre-calculate dependency tree at section creation

			if (address >= texture_cache_range.first &&
				address < texture_cache_range.second)
			{
				std::lock_guard<std::mutex> lock(m_section_mutex);

				for (int i = 0; i < m_texture_cache.size(); ++i)
				{
					auto &tex = m_texture_cache[i];
					if (!tex.is_locked()) continue;

					auto overlapped = tex.overlaps_page(trampled_range, address);
					if (std::get<0>(overlapped))
					{
						auto &new_range = std::get<1>(overlapped);

						if (new_range.first != trampled_range.first ||
							new_range.second != trampled_range.second)
						{
							trampled_range = new_range;
							i = 0;
						}

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

				for (int i = 0; i < m_rtt_cache.size(); ++i)
				{
					auto &rtt = m_rtt_cache[i];
					if (rtt.is_dirty() || !rtt.is_locked()) continue;

					auto overlapped = rtt.overlaps_page(trampled_range, address);
					if (std::get<0>(overlapped))
					{
						auto &new_range = std::get<1>(overlapped);

						if (new_range.first != trampled_range.first ||
							new_range.second != trampled_range.second)
						{
							trampled_range = new_range;
							i = 0;
						}

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

		bool flush_section(u32 address);

		void clear_temporary_surfaces()
		{
			for (u32 &id : m_temporary_surfaces)
			{
				glDeleteTextures(1, &id);
			}

			m_temporary_surfaces.clear();
		}

		bool upload_scaled_image(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate, gl_render_targets &m_rtts)
		{
			//Since we will have dst in vram, we can 'safely' ignore the swizzle flag
			//TODO: Verify correct behavior
			
			//if (dst.swizzled)
				//return false;

			bool src_is_render_target = false;	//TODO
			bool dst_is_render_target = false;	//TODO
			bool dst_is_argb8 = (dst.format == rsx::blit_engine::transfer_destination_format::a8r8g8b8);
			bool src_is_argb8 = (src.format == rsx::blit_engine::transfer_source_format::a8r8g8b8);

			GLenum src_gl_sized_format = src_is_argb8? GL_RGBA8: GL_RGB565;
			GLenum src_gl_format = src_is_argb8 ? GL_BGRA : GL_RGB;
			GLenum src_gl_type = src_is_argb8? GL_UNSIGNED_INT_8_8_8_8: GL_UNSIGNED_SHORT_5_6_5;

			u32 source_texture = 0;
			u32 dest_texture = 0;

			const u32 src_address = (u32)((u64)src.pixels - (u64)vm::base(0));
			const u32 dst_address = (u32)((u64)dst.pixels - (u64)vm::base(0));

			//Check if src/dst are parts of render targets
			surface_subresource src_subres = m_rtts.get_surface_subresource_if_applicable(src_address, src.width, src.height, src.pitch, true, true);
			src_is_render_target = src_subres.surface != nullptr;

			//Prepare areas and offsets
			//Copy from [src.offset_x, src.offset_y] a region of [clip.width, clip.height]
			//Stretch onto [dst.offset_x, y] with clipping performed on the source region
			//The implementation here adds the inverse scaled clip dimensions onto the source to completely bypass final clipping step

			float scale_x = (f32)dst.width / src.width;
			float scale_y = (f32)dst.height / src.height;

			//Clip offset is unused if the clip offsets are reprojected onto the source
			position2i clip_offset = {0, 0};//{ dst.clip_x, dst.clip_y };
			position2i dst_offset = { dst.offset_x, dst.offset_y };

			size2i clip_dimensions = { dst.clip_width, dst.clip_height };
			const size2i dst_dimensions = { dst.pitch / (dst_is_argb8 ? 4 : 2), dst.height };

			//Offset in x and y for src is 0 (it is already accounted for when getting pixels_src)
			//Reproject final clip onto source...
			const u16 src_w = clip_dimensions.width / scale_x;
			const u16 src_h = clip_dimensions.height / scale_y;

			areai src_area = { 0, 0, src_w, src_h };
			areai dst_area = { 0, 0, dst.clip_width, dst.clip_height };

			//clip_x and clip_y are not insets into the scaled region!!
			//They seem to be absolute coordinates into the full dst texture
			//Tested with persona 4 arena
			if (dst.clip_x != dst.offset_x || dst.clip_y != dst.offset_y)
			{
				const s16 clip_delta_x = dst.clip_x - dst.offset_x;
				const s16 clip_delta_y = dst.clip_y - dst.offset_x;

				if (clip_delta_x || clip_delta_y)
				{
					//Reproject clip offsets onto source
					const s16 scaled_clip_offset_x = clip_delta_x / scale_x;
					const s16 scaled_clip_offset_y = clip_delta_y / scale_y;

					src_area.x1 += scaled_clip_offset_x;
					src_area.x2 += scaled_clip_offset_x;
					src_area.y1 += scaled_clip_offset_y;
					src_area.y2 += scaled_clip_offset_y;
				}
			}

			//Create source texture if does not exist
			if (!src_is_render_target)
			{
				auto preloaded_texture = find_texture(src_address, src.width, src.slice_h, 1);

				if (preloaded_texture != nullptr)
				{
					source_texture = preloaded_texture->id();
				}
				else
				{
					flush_section(src_address);

					glGenTextures(1, &source_texture);
					glBindTexture(GL_TEXTURE_2D, source_texture);
					glTexStorage2D(GL_TEXTURE_2D, 1, src_gl_sized_format, src.width, src.slice_h);
					glPixelStorei(GL_UNPACK_ROW_LENGTH, src.pitch / (src_is_argb8 ? 4 : 2));
					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					glPixelStorei(GL_UNPACK_SWAP_BYTES, !src_is_argb8);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, src.width, src.slice_h, src_gl_format, src_gl_type, src.pixels);

					std::lock_guard<std::mutex> lock(m_section_mutex);
					
					auto section = create_texture(source_texture, src_address, src.pitch * src.slice_h, src.width, src.slice_h, 1);
					section.protect(utils::protection::ro);
					section.set_dirty(false);
				}
			}
			else
			{
				if (src_subres.w != clip_dimensions.width ||
					src_subres.h != clip_dimensions.height)
				{
					f32 subres_scaling_x = (f32)src.pitch / src_subres.surface->get_native_pitch();
					
					dst_area.x2 = (src_subres.w * scale_x * subres_scaling_x);
					dst_area.y2 = (src_subres.h * scale_y);
				}

				src_area.x2 = src_subres.w;				
				src_area.y2 = src_subres.h;

				src_area.x1 += src_subres.x;
				src_area.x2 += src_subres.x;
				src_area.y1 += src_subres.y;
				src_area.y2 += src_subres.y;

				source_texture = src_subres.surface->id();
			}

			surface_subresource dst_subres = m_rtts.get_surface_subresource_if_applicable(dst_address, dst.width, dst.clip_height, dst.pitch, true, true);
			dst_is_render_target = dst_subres.surface != nullptr;

			if (!dst_is_render_target)
			{
				auto cached_dest = find_texture(dst.rsx_address, dst.pitch * dst.clip_height);

				if (cached_dest)
				{
					dest_texture = cached_dest->id();

					//TODO: Move this algo into utils since it is used alot
					const u32 address_offset = dst.rsx_address - cached_dest->get_section_base();

					const u16 bpp = dst_is_argb8 ? 4 : 2;
					const u16 offset_y = address_offset / dst.pitch;
					const u16 offset_x = address_offset % dst.pitch;

					dst_offset.x += offset_x / bpp;
					dst_offset.y += offset_y;
				}
			}
			else
			{
				dst_offset.x = dst_subres.x;
				dst_offset.y = dst_subres.y;

				dest_texture = dst_subres.surface->id();
			}

			u32 texture_id = m_hw_blitter.scale_image(source_texture, dest_texture, src_area, dst_area, dst_offset, clip_offset,
					dst_dimensions, clip_dimensions, dst_is_argb8, interpolate);

			if (dest_texture)
				return true;

			std::lock_guard<std::mutex> lock(m_section_mutex);

			cached_texture_section &cached = create_texture(texture_id, dst.rsx_address, dst.pitch * dst.clip_height, dst.width, dst.clip_height, 1);
			cached.protect(utils::protection::ro);
			cached.set_dirty(false);

			return true;
		}
	};
}