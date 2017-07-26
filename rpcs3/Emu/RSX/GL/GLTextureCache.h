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

#include "Emu/System.h"
#include "GLRenderTargets.h"
#include "../Common/TextureUtils.h"
#include "../../Memory/vm.h"
#include "../rsx_utils.h"

class GLGSRender;

namespace gl
{
	class texture_cache
	{
	public:
		class cached_texture_section : public rsx::buffered_section
		{
		private:
			fence m_fence;
			u32 pbo_id = 0;
			u32 pbo_size = 0;

			u32 vram_texture = 0;

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

			void init_buffer()
			{
				if (pbo_id)
				{
					glDeleteBuffers(1, &pbo_id);
					pbo_id = 0;
					pbo_size = 0;
				}

				glGenBuffers(1, &pbo_id);

				const u32 buffer_size = align(cpu_address_range, 4096);
				glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_id);
				glBufferStorage(GL_PIXEL_PACK_BUFFER, buffer_size, nullptr, GL_MAP_READ_BIT);

				pbo_size = buffer_size;
			}

		public:

			void reset(const u32 base, const u32 size, const bool flushable)
			{
				rsx::protection_policy policy = g_cfg.video.strict_rendering_mode ? rsx::protection_policy::protect_policy_full_range : rsx::protection_policy::protect_policy_one_page;
				rsx::buffered_section::reset(base, size, policy);
	
				if (flushable)
					init_buffer();
				
				flushed = false;
				copied = false;

				vram_texture = 0;
			}

			void create_read_only(const u32 id, const u32 width, const u32 height)
			{
				//Only to be used for ro memory, we dont care about most members, just dimensions and the vram texture handle
				current_width = width;
				current_height = height;
				vram_texture = id;

				current_pitch = 0;
				real_pitch = 0;
			}

			bool matches(const u32 rsx_address, const u32 rsx_size)
			{
				return rsx::buffered_section::matches(rsx_address, rsx_size);
			}

			bool matches(const u32 rsx_address, const u32 width, const u32 height)
			{
				if (cpu_address_base == rsx_address && !dirty)
				{
					//Mostly only used to debug; matches textures without checking dimensions
					if (width == 0 && height == 0)
						return true;

					return (current_width == width && current_height == height);
				}

				return false;
			}

			void set_dimensions(u32 width, u32 height, u32 pitch)
			{
				current_width = width;
				current_height = height;
				current_pitch = pitch;

				real_pitch = width * get_pixel_size(format, type);
			}

			void set_format(const texture::format gl_format, const texture::type gl_type, const bool swap_bytes)
			{
				format = gl_format;
				type = gl_type;
				pack_unpack_swap_bytes = swap_bytes;

				real_pitch = current_width * get_pixel_size(format, type);
			}

			void set_source(gl::texture &source)
			{
				vram_texture = source.id();
			}

			void copy_texture()
			{
				if (!glIsTexture(vram_texture))
				{
					LOG_ERROR(RSX, "Attempted to download rtt texture, but texture handle was invalid! (0x%X)", vram_texture);
					return;
				}

				glPixelStorei(GL_PACK_SWAP_BYTES, pack_unpack_swap_bytes);
				glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_id);

				if (get_driver_caps().EXT_dsa_supported)
					glGetTextureImageEXT(vram_texture, GL_TEXTURE_2D, 0, (GLenum)format, (GLenum)type, nullptr);
				else
					glGetTextureImage(vram_texture, 0, (GLenum)format, (GLenum)type, pbo_size, nullptr);

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
					//TODO: Fall back to bilinear filtering if samples > 2

					const u8 pixel_size = get_pixel_size(format, type);
					const u8 samples = current_pitch / real_pitch;
					rsx::scale_image_nearest(dst, const_cast<const void*>(data), current_width, current_height, current_pitch, real_pitch, pixel_size, samples);
				}

				glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
				glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
				protect(utils::protection::ro);
			}

			void destroy()
			{
				if (locked)
					unprotect();

				if (pbo_id == 0)
				{
					//Read-only texture, destroy texture memory
					glDeleteTextures(1, &vram_texture);
					vram_texture = 0;
				}
				else
				{
					//Destroy pbo cache since vram texture is managed elsewhere
					glDeleteBuffers(1, &pbo_id);
					pbo_id = 0;
					pbo_size = 0;
				}

				m_fence.destroy();
			}

			bool is_flushed() const
			{
				return flushed;
			}

			void set_flushed(const bool state)
			{
				flushed = state;
			}

			void set_copied(const bool state)
			{
				copied = state;
			}

			bool is_empty() const
			{
				return vram_texture == 0;
			}

			const u32 id()
			{
				return vram_texture;
			}

			std::tuple<u32, u32> get_dimensions()
			{
				return std::make_tuple(current_width, current_height);
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

				GLboolean scissor_test_enabled = glIsEnabled(GL_SCISSOR_TEST);
				if (scissor_test_enabled)
					glDisable(GL_SCISSOR_TEST);

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

				if (scissor_test_enabled)
					glEnable(GL_SCISSOR_TEST);

				glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
				return dst_tex;
			}
		};

	private:
		std::vector<cached_texture_section> read_only_memory_sections;
		std::vector<cached_texture_section> no_access_memory_sections;
		std::vector<u32> m_temporary_surfaces;

		std::pair<u32, u32> read_only_range = std::make_pair(0xFFFFFFFF, 0);
		std::pair<u32, u32> no_access_range = std::make_pair(0xFFFFFFFF, 0);

		blitter m_hw_blitter;

		std::mutex m_section_mutex;

		GLGSRender *m_renderer;
		std::thread::id m_renderer_thread;

		cached_texture_section *find_texture_from_dimensions(u32 texaddr, u32 w, u32 h)
		{
			std::lock_guard<std::mutex> lock(m_section_mutex);

			for (cached_texture_section &tex : read_only_memory_sections)
			{
				if (tex.matches(texaddr, w, h) && !tex.is_dirty())
					return &tex;
			}

			return nullptr;
		}

		/**
		 * Searches for a texture from read_only memory sections
		 * Texture origin + size must be a subsection of the existing texture
		 */
		cached_texture_section *find_texture_from_range(u32 texaddr, u32 range)
		{
			std::lock_guard<std::mutex> lock(m_section_mutex);

			auto test = std::make_pair(texaddr, range);
			for (cached_texture_section &tex : read_only_memory_sections)
			{
				if (tex.get_section_base() > texaddr)
					continue;

				if (tex.overlaps(test, true) && !tex.is_dirty())
					return &tex;
			}

			return nullptr;
		}

		cached_texture_section& create_texture(u32 id, u32 texaddr, u32 texsize, u32 w, u32 h)
		{
			for (cached_texture_section &tex : read_only_memory_sections)
			{
				if (tex.is_dirty())
				{
					tex.destroy();
					tex.reset(texaddr, texsize, false);
					tex.create_read_only(id, w, h);
					
					read_only_range = tex.get_min_max(read_only_range);
					return tex;
				}
			}

			cached_texture_section tex;
			tex.reset(texaddr, texsize, false);
			tex.create_read_only(id, w, h);
			read_only_range = tex.get_min_max(read_only_range);

			read_only_memory_sections.push_back(tex);
			return read_only_memory_sections.back();
		}

		void clear()
		{
			for (cached_texture_section &tex : read_only_memory_sections)
			{
				tex.destroy();
			}

			for (cached_texture_section &tex : no_access_memory_sections)
			{
				tex.destroy();
			}

			read_only_memory_sections.resize(0);
			no_access_memory_sections.resize(0);

			clear_temporary_surfaces();
		}

		cached_texture_section* find_cached_rtt_section(u32 base, u32 size)
		{
			for (cached_texture_section &rtt : no_access_memory_sections)
			{
				if (rtt.matches(base, size))
				{
					return &rtt;
				}
			}

			return nullptr;
		}

		cached_texture_section *create_locked_view_of_section(u32 base, u32 size)
		{
			cached_texture_section *region = find_cached_rtt_section(base, size);

			if (!region)
			{
				for (cached_texture_section &rtt : no_access_memory_sections)
				{
					if (rtt.is_dirty())
					{
						rtt.reset(base, size, true);
						rtt.protect(utils::protection::no);
						region = &rtt;
						break;
					}
				}

				if (!region)
				{
					cached_texture_section section;
					section.reset(base, size, true);
					section.set_dirty(true);
					section.protect(utils::protection::no);

					no_access_memory_sections.push_back(section);
					region = &no_access_memory_sections.back();
				}

				no_access_range = region->get_min_max(no_access_range);
			}
			else
			{
				//This section view already exists
				if (region->get_section_size() != size)
				{
					region->unprotect();
					region->reset(base, size, true);
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
			
			const u32 format = tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
			const u32 tex_width = tex.width();
			const u32 tex_height = tex.height();
			const u32 native_pitch = (tex_width * get_format_block_size_in_bytes(format));
			const u32 tex_pitch = (tex.pitch() == 0)? native_pitch: tex.pitch();

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
			gl::render_target *texptr = nullptr;
			if (texptr = m_rtts.get_texture_from_render_target_if_applicable(texaddr))
			{
				for (const auto& tex : m_rtts.m_bound_render_targets)
				{
					if (std::get<0>(tex) == texaddr)
					{
						if (g_cfg.video.strict_rendering_mode)
						{
							LOG_WARNING(RSX, "Attempting to sample a currently bound render target @ 0x%x", texaddr);
							create_temporary_subresource(texptr->id(), (GLenum)texptr->get_compatible_internal_format(), 0, 0, texptr->width(), texptr->height());
							return;
						}
						else
						{
							//issue a texture barrier to ensure previous writes are visible
							auto &caps = gl::get_driver_caps();

							if (caps.ARB_texture_barrier_supported)
								glTextureBarrier();
							else if (caps.NV_texture_barrier_supported)
								glTextureBarrierNV();

							break;
						}
					}
				}

				texptr->bind();
				return;
			}

			if (texptr = m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr))
			{
				if (texaddr == std::get<0>(m_rtts.m_bound_depth_stencil))
				{
					if (g_cfg.video.strict_rendering_mode)
					{
						LOG_WARNING(RSX, "Attempting to sample a currently bound depth surface @ 0x%x", texaddr);
						create_temporary_subresource(texptr->id(), (GLenum)texptr->get_compatible_internal_format(), 0, 0, texptr->width(), texptr->height());
						return;
					}
					else
					{
						//issue a texture barrier to ensure previous writes are visible
						auto &caps = gl::get_driver_caps();

						if (caps.ARB_texture_barrier_supported)
							glTextureBarrier();
						else if (caps.NV_texture_barrier_supported)
							glTextureBarrierNV();
					}
				}

				texptr->bind();
				return;
			}

			/**
			 * Check if we are re-sampling a subresource of an RTV/DSV texture, bound or otherwise
			 * (Turbo: Super Stunt Squad does this; bypassing the need for a sync object)
			 * The engine does not read back the texture resource through cell, but specifies a texture location that is
			 * a bound render target. We can bypass the expensive download in this case
			 */

			const f32 internal_scale = (f32)tex_pitch / native_pitch;
			const u32 internal_width = (const u32)(tex_width * internal_scale);

			const surface_subresource rsc = m_rtts.get_surface_subresource_if_applicable(texaddr, internal_width, tex_height, tex_pitch, true);
			if (rsc.surface)
			{
				//Check that this region is not cpu-dirty before doing a copy
				//This section is guaranteed to have a locking section *if* this bit has been bypassed before

				//Is this really necessary?
				bool upload_from_cpu = false;

				for (cached_texture_section &section : no_access_memory_sections)
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
						u32 bound_index = ~0U;

						bool dst_is_compressed = (format == CELL_GCM_TEXTURE_COMPRESSED_DXT1 || format == CELL_GCM_TEXTURE_COMPRESSED_DXT23 || format == CELL_GCM_TEXTURE_COMPRESSED_DXT45);

						if (!dst_is_compressed)
						{
							GLenum src_format = (GLenum)rsc.surface->get_internal_format();
							GLenum dst_format = std::get<0>(get_format_type(format));

							if (src_format != dst_format)
							{
								LOG_WARNING(RSX, "Sampling from a section of a render target, but formats might be incompatible (0x%X vs 0x%X)", src_format, dst_format);
							}
						}
						else
						{
							LOG_WARNING(RSX, "Surface blit from a compressed texture");
						}

						if (!rsc.is_bound || !g_cfg.video.strict_rendering_mode)
						{
							if (rsc.w == tex_width && rsc.h == tex_height)
							{
								if (rsc.is_bound)
								{
									LOG_WARNING(RSX, "Sampling from a currently bound render target @ 0x%x", texaddr);

									auto &caps = gl::get_driver_caps();
									if (caps.ARB_texture_barrier_supported)
										glTextureBarrier();
									else if (caps.NV_texture_barrier_supported)
										glTextureBarrierNV();
								}

								rsc.surface->bind();
							}
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

			cached_texture_section *cached_texture = find_texture_from_dimensions(texaddr, tex_width, tex_height);
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
			cached_texture = find_texture_from_range(texaddr, range);
			if (cached_texture)
			{
				const u32 address_offset = texaddr - cached_texture->get_section_base();
				const u32 format = tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
				const GLenum ifmt = gl::get_sized_internal_format(format);

				u16 offset_x = 0, offset_y = 0;

				if (address_offset)
				{
					const u32 bpp = get_format_block_size_in_bytes(format);

					offset_y = address_offset / tex_pitch;
					offset_x = address_offset % tex_pitch;

					offset_x /= bpp;
					offset_y /= bpp;
				}

				u32 texture_id = create_temporary_subresource(cached_texture->id(), ifmt, offset_x, offset_y, tex_width, tex_height);
				if (texture_id) return;
			}

			gl_texture.init(index, tex);

			std::lock_guard<std::mutex> lock(m_section_mutex);

			cached_texture_section &cached = create_texture(gl_texture.id(), texaddr, (const u32)get_texture_size(tex), tex_width, tex_height);
			cached.protect(utils::protection::ro);
			cached.set_dirty(false);

			//external gl::texture objects should always be undefined/uninitialized!
			gl_texture.set_id(0);
		}

		void save_rtt(u32 base, u32 size)
		{
			std::lock_guard<std::mutex> lock(m_section_mutex);

			cached_texture_section *region = find_cached_rtt_section(base, size);

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

			cached_texture_section *region = create_locked_view_of_section(base, size);

			if (!region->matches(base, size))
			{
				//This memory region overlaps our own region, but does not match it exactly
				if (region->is_locked())
					region->unprotect();

				region->reset(base, size, true);
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
			cached_texture_section *rtt = find_cached_rtt_section(address, range);

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

			if (address >= read_only_range.first &&
				address < read_only_range.second)
			{
				std::lock_guard<std::mutex> lock(m_section_mutex);

				for (int i = 0; i < read_only_memory_sections.size(); ++i)
				{
					auto &tex = read_only_memory_sections[i];
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

			if (address >= no_access_range.first &&
				address < no_access_range.second)
			{
				std::lock_guard<std::mutex> lock(m_section_mutex);

				for (int i = 0; i < no_access_memory_sections.size(); ++i)
				{
					auto &tex = no_access_memory_sections[i];
					if (tex.is_dirty() || !tex.is_locked()) continue;

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

			return response;
		}

		void invalidate_range(u32 base, u32 size)
		{
			std::lock_guard<std::mutex> lock(m_section_mutex);
			std::pair<u32, u32> range = std::make_pair(base, size);

			if (base < read_only_range.second &&
				(base + size) >= read_only_range.first)
			{
				for (cached_texture_section &tex : read_only_memory_sections)
				{
					if (!tex.is_dirty() && tex.overlaps(range))
						tex.destroy();
				}
			}

			if (base < no_access_range.second &&
				(base + size) >= no_access_range.first)
			{
				for (cached_texture_section &tex : no_access_memory_sections)
				{
					if (!tex.is_dirty() && tex.overlaps(range))
					{
						tex.unprotect();
						tex.set_dirty(true);
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

			bool src_is_render_target = false;
			bool dst_is_render_target = false;
			bool dst_is_argb8 = (dst.format == rsx::blit_engine::transfer_destination_format::a8r8g8b8);
			bool src_is_argb8 = (src.format == rsx::blit_engine::transfer_source_format::a8r8g8b8);

			GLenum src_gl_sized_format = src_is_argb8? GL_RGBA8: GL_RGB565;
			GLenum src_gl_format = src_is_argb8 ? GL_BGRA : GL_RGB;
			GLenum src_gl_type = src_is_argb8? GL_UNSIGNED_INT_8_8_8_8: GL_UNSIGNED_SHORT_5_6_5;

			u32 vram_texture = 0;
			u32 dest_texture = 0;

			const u32 src_address = (u32)((u64)src.pixels - (u64)vm::base(0));
			const u32 dst_address = (u32)((u64)dst.pixels - (u64)vm::base(0));

			//Check if src/dst are parts of render targets
			surface_subresource dst_subres = m_rtts.get_surface_subresource_if_applicable(dst_address, dst.width, dst.clip_height, dst.pitch, true, true);
			dst_is_render_target = dst_subres.surface != nullptr;

			u16 max_dst_width = dst.width;
			u16 max_dst_height = dst.height;

			//Prepare areas and offsets
			//Copy from [src.offset_x, src.offset_y] a region of [clip.width, clip.height]
			//Stretch onto [dst.offset_x, y] with clipping performed on the source region
			//The implementation here adds the inverse scaled clip dimensions onto the source to completely bypass final clipping step

			float scale_x = (f32)dst.width / src.width;
			float scale_y = (f32)dst.height / src.height;

			//Clip offset is unused if the clip offsets are reprojected onto the source
			position2i clip_offset = { 0, 0 };//{ dst.clip_x, dst.clip_y };
			position2i dst_offset = { dst.offset_x, dst.offset_y };

			size2i clip_dimensions = { dst.clip_width, dst.clip_height };
			const size2i dst_dimensions = { dst.pitch / (dst_is_argb8 ? 4 : 2), dst.height };

			//Offset in x and y for src is 0 (it is already accounted for when getting pixels_src)
			//Reproject final clip onto source...
			const u16 src_w = (const u16)((f32)clip_dimensions.width / scale_x);
			const u16 src_h = (const u16)((f32)clip_dimensions.height / scale_y);

			areai src_area = { 0, 0, src_w, src_h };
			areai dst_area = { 0, 0, dst.clip_width, dst.clip_height };

			//If destination is neither a render target nor an existing texture in VRAM
			//its possible that this method is being used to perform a memcpy into RSX memory, so we check
			//parameters. Whenever a simple memcpy can get the job done, use it instead.
			//Dai-3-ji Super Robot Taisen for example uses this to copy program code to GPU RAM
			
			bool is_memcpy = false;
			u32 memcpy_bytes_length = 0;
			if (dst_is_argb8 == src_is_argb8 && !dst.swizzled)
			{
				if ((src.slice_h == 1 && dst.clip_height == 1) ||
					(dst.clip_width == src.width && dst.clip_height == src.slice_h && src.pitch == dst.pitch))
				{
					const u8 bpp = dst_is_argb8 ? 4 : 2;
					is_memcpy = true;
					memcpy_bytes_length = dst.clip_width * bpp * dst.clip_height;
				}
			}

			if (!dst_is_render_target)
			{
				//First check if this surface exists in VRAM with exact dimensions
				//Since scaled GPU resources are not invalidated by the CPU, we need to reuse older surfaces if possible
				auto cached_dest = find_texture_from_dimensions(dst.rsx_address, dst_dimensions.width, dst_dimensions.height);

				//Check for any available region that will fit this one
				if (!cached_dest) cached_dest = find_texture_from_range(dst.rsx_address, dst.pitch * dst.clip_height);

				if (cached_dest)
				{
					//TODO: Verify that the new surface will fit
					dest_texture = cached_dest->id();

					//TODO: Move this code into utils since it is used alot
					const u32 address_offset = dst.rsx_address - cached_dest->get_section_base();

					const u16 bpp = dst_is_argb8 ? 4 : 2;
					const u16 offset_y = address_offset / dst.pitch;
					const u16 offset_x = address_offset % dst.pitch;

					dst_offset.x += offset_x / bpp;
					dst_offset.y += offset_y;

					std::tie(max_dst_width, max_dst_height) = cached_dest->get_dimensions();
				}
				else if (is_memcpy)
				{
					memcpy(dst.pixels, src.pixels, memcpy_bytes_length);
					return true;
				}
			}
			else
			{
				dst_offset.x = dst_subres.x;
				dst_offset.y = dst_subres.y;

				dest_texture = dst_subres.surface->id();

				auto dims = dst_subres.surface->get_dimensions();
				max_dst_width = dims.first;
				max_dst_height = dims.second;

				if (is_memcpy)
				{
					//Some render target descriptions are actually invalid
					//Confirm this is a flushable RTT
					const auto rsx_pitch = dst_subres.surface->get_rsx_pitch();
					const auto native_pitch = dst_subres.surface->get_native_pitch();

					if (rsx_pitch <= 64 && native_pitch != rsx_pitch)
					{
						memcpy(dst.pixels, src.pixels, memcpy_bytes_length);
						return true;
					}
				}
			}

			surface_subresource src_subres = m_rtts.get_surface_subresource_if_applicable(src_address, src.width, src.height, src.pitch, true, true);
			src_is_render_target = src_subres.surface != nullptr;

			//Create source texture if does not exist
			if (!src_is_render_target)
			{
				auto preloaded_texture = find_texture_from_dimensions(src_address, src.width, src.slice_h);

				if (preloaded_texture != nullptr)
				{
					vram_texture = preloaded_texture->id();
				}
				else
				{
					flush_section(src_address);

					GLboolean swap_bytes = !src_is_argb8;
					if (dst.swizzled)
					{
						//TODO: Check results against 565 textures
						if (src_is_argb8)
						{
							src_gl_format = GL_RGBA;
							swap_bytes = true;
						}
						else
						{
							LOG_ERROR(RSX, "RGB565 swizzled texture upload found");
						}
					}

					glGenTextures(1, &vram_texture);
					glBindTexture(GL_TEXTURE_2D, vram_texture);
					glTexStorage2D(GL_TEXTURE_2D, 1, src_gl_sized_format, src.width, src.slice_h);
					glPixelStorei(GL_UNPACK_ROW_LENGTH, src.pitch / (src_is_argb8 ? 4 : 2));
					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					glPixelStorei(GL_UNPACK_SWAP_BYTES, swap_bytes);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, src.width, src.slice_h, src_gl_format, src_gl_type, src.pixels);

					std::lock_guard<std::mutex> lock(m_section_mutex);
					
					auto &section = create_texture(vram_texture, src_address, src.pitch * src.slice_h, src.width, src.slice_h);
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
					
					dst_area.x2 = (int)(src_subres.w * scale_x * subres_scaling_x);
					dst_area.y2 = (int)(src_subres.h * scale_y);
				}

				src_area.x2 = src_subres.w;				
				src_area.y2 = src_subres.h;

				src_area.x1 += src_subres.x;
				src_area.x2 += src_subres.x;
				src_area.y1 += src_subres.y;
				src_area.y2 += src_subres.y;

				vram_texture = src_subres.surface->id();
			}

			//Validate clip offsets (Persona 4 Arena at 720p)
			//Check if can fit
			//NOTE: It is possible that the check is simpler (if (clip_x >= clip_width))
			//Needs verification
			if ((dst.offset_x + dst.clip_x + dst.clip_width) > max_dst_width) dst.clip_x = 0;
			if ((dst.offset_y + dst.clip_y + dst.clip_height) > max_dst_height) dst.clip_y = 0;

			if (dst.clip_x || dst.clip_y)
			{
				//Reproject clip offsets onto source
				const u16 scaled_clip_offset_x = (const u16)((f32)dst.clip_x / scale_x);
				const u16 scaled_clip_offset_y = (const u16)((f32)dst.clip_y / scale_y);

				src_area.x1 += scaled_clip_offset_x;
				src_area.x2 += scaled_clip_offset_x;
				src_area.y1 += scaled_clip_offset_y;
				src_area.y2 += scaled_clip_offset_y;
			}

			u32 texture_id = m_hw_blitter.scale_image(vram_texture, dest_texture, src_area, dst_area, dst_offset, clip_offset,
					dst_dimensions, clip_dimensions, dst_is_argb8, interpolate);

			if (dest_texture)
				return true;

			//TODO: Verify if any titles ever scale into CPU memory. It defeats the purpose of uploading data to the GPU, but it could happen
			//If so, add this texture to the no_access queue not the read_only queue
			std::lock_guard<std::mutex> lock(m_section_mutex);

			cached_texture_section &cached = create_texture(texture_id, dst.rsx_address, dst.pitch * dst.clip_height, dst.width, dst.clip_height);
			//These textures are completely GPU resident so we dont watch for CPU access
			//There's no data to be fetched from the CPU
			//Its is possible for a title to attempt to read from the region, but the CPU path should be used in such cases
			cached.protect(utils::protection::rw);
			cached.set_dirty(false);

			return true;
		}
	};
}
