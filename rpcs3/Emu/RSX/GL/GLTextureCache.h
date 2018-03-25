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

#include "Utilities/mutex.h"
#include "Emu/System.h"
#include "GLRenderTargets.h"
#include "GLOverlays.h"
#include "../Common/TextureUtils.h"
#include "../Common/texture_cache.h"
#include "../../Memory/vm.h"
#include "../rsx_utils.h"

class GLGSRender;

namespace gl
{
	class blitter;

	extern GLenum get_sized_internal_format(u32);
	extern blitter *g_hw_blitter;

	class blitter
	{
		fbo blit_src;
		fbo blit_dst;

	public:

		void init()
		{
			blit_src.create();
			blit_dst.create();
		}

		void destroy()
		{
			blit_dst.remove();
			blit_src.remove();
		}

		u32 scale_image(u32 src, u32 dst, areai src_rect, areai dst_rect, bool linear_interpolation, bool is_depth_copy)
		{
			s32 old_fbo = 0;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);

			u32 dst_tex = dst;
			filter interp = linear_interpolation ? filter::linear : filter::nearest;

			GLenum attachment = is_depth_copy ? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0;

			blit_src.bind();
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, src, 0);
			blit_src.check();

			blit_dst.bind();
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, dst_tex, 0);
			blit_dst.check();

			GLboolean scissor_test_enabled = glIsEnabled(GL_SCISSOR_TEST);
			if (scissor_test_enabled)
				glDisable(GL_SCISSOR_TEST);

			blit_src.blit(blit_dst, src_rect, dst_rect, is_depth_copy ? buffers::depth : buffers::color, interp);

			blit_src.bind();
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, GL_NONE, 0);

			blit_dst.bind();
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, GL_NONE, 0);

			if (scissor_test_enabled)
				glEnable(GL_SCISSOR_TEST);

			glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
			return dst_tex;
		}
	};

	class cached_texture_section : public rsx::cached_texture_section
	{
	private:
		fence m_fence;
		u32 pbo_id = 0;
		u32 pbo_size = 0;

		u32 vram_texture = 0;
		u32 scaled_texture = 0;

		bool is_depth = false;

		texture::format format = texture::format::rgba;
		texture::type type = texture::type::ubyte;
		rsx::surface_antialiasing aa_mode = rsx::surface_antialiasing::center_1_sample;

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
			default:
				LOG_ERROR(RSX, "Unsupported texture type");
			}

			switch (fmt_)
			{
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
			const f32 resolution_scale = (context == rsx::texture_upload_context::framebuffer_storage? rsx::get_resolution_scale() : 1.f);
			const u32 real_buffer_size = (resolution_scale <= 1.f) ? cpu_address_range : (u32)(resolution_scale * resolution_scale * cpu_address_range);
			const u32 buffer_size = align(real_buffer_size, 4096);

			if (pbo_id)
			{
				if (pbo_size >= buffer_size)
					return;

				glDeleteBuffers(1, &pbo_id);
				pbo_id = 0;
				pbo_size = 0;
			}

			glGenBuffers(1, &pbo_id);

			glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_id);
			glBufferStorage(GL_PIXEL_PACK_BUFFER, buffer_size, nullptr, GL_MAP_READ_BIT);

			pbo_size = buffer_size;
		}

	public:

		void reset(u32 base, u32 size, bool /*flushable*/=false)
		{
			rsx::protection_policy policy = g_cfg.video.strict_rendering_mode ? rsx::protection_policy::protect_policy_full_range : rsx::protection_policy::protect_policy_conservative;
			rsx::buffered_section::reset(base, size, policy);

			flushed = false;
			synchronized = false;
			is_depth = false;

			vram_texture = 0;
		}

		void create(u16 w, u16 h, u16 depth, u16 mipmaps, void*,
				gl::texture* image, u32 rsx_pitch, bool read_only,
				gl::texture::format gl_format, gl::texture::type gl_type, bool swap_bytes)
		{
			if (read_only)
			{
				aa_mode = rsx::surface_antialiasing::center_1_sample;
			}
			else
			{
				if (pbo_id == 0)
					init_buffer();

				aa_mode = static_cast<gl::render_target*>(image)->aa_mode;
			}

			flushed = false;
			synchronized = false;
			is_depth = false;

			this->width = w;
			this->height = h;
			this->rsx_pitch = rsx_pitch;
			this->depth = depth;
			this->mipmaps = mipmaps;

			vram_texture = image->id();
			set_format(gl_format, gl_type, swap_bytes);
		}

		void create_read_only(u32 id, u32 width, u32 height, u32 depth, u32 mipmaps)
		{
			//Only to be used for ro memory, we dont care about most members, just dimensions and the vram texture handle
			this->width = width;
			this->height = height;
			this->depth = depth;
			this->mipmaps = mipmaps;
			vram_texture = id;

			rsx_pitch = 0;
			real_pitch = 0;
		}

		void make_flushable()
		{
			//verify(HERE), pbo_id == 0;
			init_buffer();
		}

		void set_dimensions(u32 width, u32 height, u32 /*depth*/, u32 pitch)
		{
			this->width = width;
			this->height = height;
			rsx_pitch = pitch;
			real_pitch = width * get_pixel_size(format, type);
		}

		void set_format(texture::format gl_format, texture::type gl_type, bool swap_bytes)
		{
			format = gl_format;
			type = gl_type;
			pack_unpack_swap_bytes = swap_bytes;

			if (format == gl::texture::format::rgba)
			{
				switch (type)
				{
				case gl::texture::type::f16:
					gcm_format = CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT;
					break;
				case gl::texture::type::f32:
					gcm_format = CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT;
					break;
				}
			}

			real_pitch = width * get_pixel_size(format, type);
		}

		void set_depth_flag(bool is_depth_fmt)
		{
			is_depth = is_depth_fmt;
		}

		void set_source(gl::texture &source)
		{
			vram_texture = source.id();
		}

		void copy_texture(bool=false)
		{
			if (!glIsTexture(vram_texture))
			{
				LOG_ERROR(RSX, "Attempted to download rtt texture, but texture handle was invalid! (0x%X)", vram_texture);
				return;
			}

			if (!pbo_id)
			{
				init_buffer();
			}

			u32 target_texture = vram_texture;
			if ((rsx::get_resolution_scale_percent() != 100 && context == rsx::texture_upload_context::framebuffer_storage) ||
				(real_pitch != rsx_pitch))
			{
				u32 real_width = width;
				u32 real_height = height;

				switch (aa_mode)
				{
				case rsx::surface_antialiasing::center_1_sample:
					break;
				case rsx::surface_antialiasing::diagonal_centered_2_samples:
					real_width *= 2;
					break;
				default:
					real_width *= 2;
					real_height *= 2;
					break;
				}

				areai src_area = { 0, 0, 0, 0 };
				const areai dst_area = { 0, 0, (s32)real_width, (s32)real_height };

				GLenum ifmt = 0;
				glBindTexture(GL_TEXTURE_2D, vram_texture);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, (GLint*)&ifmt);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &src_area.x2);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &src_area.y2);

				if (src_area.x2 != dst_area.x2 || src_area.y2 != dst_area.y2)
				{
					if (scaled_texture != 0)
					{
						int sw, sh, fmt;
						glBindTexture(GL_TEXTURE_2D, scaled_texture);
						glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
						glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
						glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &fmt);

						if ((u32)sw != real_width || (u32)sh != real_height || (GLenum)fmt != ifmt)
						{
							glDeleteTextures(1, &scaled_texture);
							scaled_texture = 0;
						}
					}

					if (scaled_texture == 0)
					{
						glGenTextures(1, &scaled_texture);
						glBindTexture(GL_TEXTURE_2D, scaled_texture);
						glTexStorage2D(GL_TEXTURE_2D, 1, ifmt, real_width, real_height);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
					}

					bool linear_interp = false; //TODO: Make optional or detect full sized sources
					g_hw_blitter->scale_image(vram_texture, scaled_texture, src_area, dst_area, linear_interp, is_depth);
					target_texture = scaled_texture;
				}
			}

			glPixelStorei(GL_PACK_SWAP_BYTES, pack_unpack_swap_bytes);
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glPixelStorei(GL_PACK_ROW_LENGTH, 0);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_id);

			glGetError();

			if (get_driver_caps().EXT_dsa_supported)
				glGetTextureImageEXT(target_texture, GL_TEXTURE_2D, 0, (GLenum)format, (GLenum)type, nullptr);
			else
				glGetTextureImage(target_texture, 0, (GLenum)format, (GLenum)type, pbo_size, nullptr);

			if (GLenum err = glGetError())
			{
				bool recovered = false;
				if (target_texture == scaled_texture)
				{
					if (get_driver_caps().EXT_dsa_supported)
						glGetTextureImageEXT(vram_texture, GL_TEXTURE_2D, 0, (GLenum)format, (GLenum)type, nullptr);
					else
						glGetTextureImage(vram_texture, 0, (GLenum)format, (GLenum)type, pbo_size, nullptr);

					if (!glGetError())
					{
						recovered = true;
						const u32 min_dimension = cpu_address_range / rsx_pitch;
						LOG_WARNING(RSX, "Failed to read back a scaled image, but the original texture can be read back. Consider setting min scalable dimension below or equal to %d", min_dimension);
					}
				}

				if (!recovered && rsx::get_resolution_scale_percent() != 100 && context == rsx::texture_upload_context::framebuffer_storage)
				{
					LOG_ERROR(RSX, "Texture readback failed. Disable resolution scaling to get the 'Write Color Buffers' option to work properly");
				}
			}

			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

			m_fence.reset();
			synchronized = true;
		}

		void fill_texture(gl::texture* tex)
		{
			if (!synchronized)
			{
				//LOG_WARNING(RSX, "Request to fill texture rejected because contents were not read");
				return;
			}

			u32 min_width = std::min((u16)tex->width(), width);
			u32 min_height = std::min((u16)tex->height(), height);

			tex->bind();
			glPixelStorei(GL_UNPACK_SWAP_BYTES, pack_unpack_swap_bytes);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_id);
			glTexSubImage2D((GLenum)tex->get_target(), 0, 0, 0, min_width, min_height, (GLenum)format, (GLenum)type, nullptr);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		}

		bool flush()
		{
			if (flushed) return true; //Already written, ignore

			bool result = true;
			if (!synchronized)
			{
				LOG_WARNING(RSX, "Cache miss at address 0x%X. This is gonna hurt...", cpu_address_base);
				copy_texture();

				if (!synchronized)
				{
					LOG_WARNING(RSX, "Nothing to copy; Setting section to readable and moving on...");
					protect(utils::protection::ro);
					return false;
				}

				result = false;
			}

			m_fence.wait_for_signal();
			flushed = true;

			glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_id);
			void *data = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, pbo_size, GL_MAP_READ_BIT);
			u8 *dst = vm::_ptr<u8>(cpu_address_base);

			//throw if map failed since we'll segfault anyway
			verify(HERE), data != nullptr;

			bool require_manual_shuffle = false;
			if (pack_unpack_swap_bytes)
			{
				if (type == gl::texture::type::sbyte || type == gl::texture::type::ubyte)
					require_manual_shuffle = true;
			}

			if (real_pitch >= rsx_pitch || scaled_texture != 0)
			{
				memcpy(dst, data, cpu_address_range);
			}
			else
			{
				const u8 pixel_size = get_pixel_size(format, type);
				const u8 samples_u = (aa_mode == rsx::surface_antialiasing::center_1_sample) ? 1 : 2;
				const u8 samples_v = (aa_mode == rsx::surface_antialiasing::square_centered_4_samples || aa_mode == rsx::surface_antialiasing::square_rotated_4_samples) ? 2 : 1;
				rsx::scale_image_nearest(dst, const_cast<const void*>(data), width, height, rsx_pitch, real_pitch, pixel_size, samples_u, samples_v);
			}

			if (require_manual_shuffle)
			{
				//byte swapping does not work on byte types, use uint_8_8_8_8 for rgba8 instead to avoid penalty
				rsx::shuffle_texel_data_wzyx<u8>(dst, rsx_pitch, width, height);
			}

			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

			reset_write_statistics();

			return result;
		}

		void reprotect(utils::protection prot)
		{
			flushed = false;
			synchronized = false;
			protect(prot);
		}

		void destroy()
		{
			if (!locked && pbo_id == 0 && vram_texture == 0 && m_fence.is_empty())
				//Already destroyed
				return;

			if (locked)
				unprotect();

			if (pbo_id == 0)
			{
				//Read-only texture, destroy texture memory
				glDeleteTextures(1, &vram_texture);
			}
			else
			{
				//Destroy pbo cache since vram texture is managed elsewhere
				glDeleteBuffers(1, &pbo_id);

				if (scaled_texture)
					glDeleteTextures(1, &scaled_texture);
			}

			vram_texture = 0;
			scaled_texture = 0;
			pbo_id = 0;
			pbo_size = 0;

			if (!m_fence.is_empty())
				m_fence.destroy();
		}

		texture::format get_format() const
		{
			return format;
		}

		bool exists() const
		{
			return vram_texture != 0;
		}

		bool is_flushable() const
		{
			return (locked && pbo_id != 0);
		}

		bool is_flushed() const
		{
			return flushed;
		}

		bool is_synchronized() const
		{
			return synchronized;
		}

		void set_flushed(bool state)
		{
			flushed = state;
		}

		bool is_empty() const
		{
			return vram_texture == 0;
		}

		u32 get_raw_view() const
		{
			return vram_texture;
		}

		u32 get_raw_texture() const
		{
			return vram_texture;
		}

		bool is_depth_texture() const
		{
			return is_depth;
		}

		bool has_compatible_format(gl::texture* tex) const
		{
			GLenum fmt;
			glBindTexture(GL_TEXTURE_2D, vram_texture);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, (GLint*)&fmt);

			if (auto as_rtt = dynamic_cast<gl::render_target*>(tex))
			{
				const auto rtt_internal_fmt = (GLenum)as_rtt->get_compatible_internal_format();
				if (rtt_internal_fmt != fmt)
				{
					//When high precision Z is enabled, RTT depth surfaces use DEPTH32F instead of DEPTH24
					if (rtt_internal_fmt == GL_DEPTH32F_STENCIL8 && fmt == GL_DEPTH24_STENCIL8)
						return true;

					return false;
				}
				else
				{
					//Match
					return true;
				}
			}

			return (gl::texture::format)fmt == tex->get_internal_format();
		}
	};

	class texture_cache : public rsx::texture_cache<void*, cached_texture_section, u32, u32, gl::texture, gl::texture::format>
	{
	private:

		blitter m_hw_blitter;
		std::vector<u32> m_temporary_surfaces;

		cached_texture_section& create_texture(u32 id, u32 texaddr, u32 texsize, u32 w, u32 h, u32 depth, u32 mipmaps)
		{
			cached_texture_section& tex = find_cached_texture(texaddr, texsize, true, w, h, depth);
			tex.reset(texaddr, texsize, false);
			tex.create_read_only(id, w, h, depth, mipmaps);
			read_only_range = tex.get_min_max(read_only_range);
			return tex;
		}

		void clear()
		{
			for (auto &address_range : m_cache)
			{
				auto &range_data = address_range.second;
				for (auto &tex : range_data.data)
				{
					tex.destroy();
				}

				range_data.data.resize(0);
			}

			clear_temporary_subresources();
			m_unreleased_texture_objects = 0;
		}

		void clear_temporary_subresources()
		{
			for (u32 &id : m_temporary_surfaces)
			{
				glDeleteTextures(1, &id);
			}

			m_temporary_surfaces.resize(0);
		}

		u32 create_temporary_subresource_impl(u32 src_id, GLenum sized_internal_fmt, GLenum dst_type, u32 gcm_format,
				u16 x, u16 y, u16 width, u16 height, const texture_channel_remap_t& remap, bool copy)
		{
			u32 dst_id = 0;

			if (sized_internal_fmt == GL_NONE)
				sized_internal_fmt = gl::get_sized_internal_format(gcm_format);

			GLenum ifmt;
			glBindTexture(GL_TEXTURE_2D, src_id);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, (GLint*)&ifmt);

			switch (ifmt)
			{
			case GL_DEPTH_COMPONENT16:
			case GL_DEPTH_COMPONENT24:
			case GL_DEPTH24_STENCIL8:
				sized_internal_fmt = ifmt;
				break;
			}

			glGenTextures(1, &dst_id);
			glBindTexture(dst_type, dst_id);

			if (dst_type == GL_TEXTURE_2D)
				glTexStorage2D(GL_TEXTURE_2D, 1, sized_internal_fmt, width, height);
			else if (dst_type == GL_TEXTURE_1D)
				glTexStorage1D(GL_TEXTURE_1D, 1, sized_internal_fmt, width);

			glTexParameteri(dst_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(dst_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(dst_type, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(dst_type, GL_TEXTURE_MAX_LEVEL, 0);

			m_temporary_surfaces.push_back(dst_id);

			if (copy)
			{
				//Empty GL_ERROR
				glGetError();

				glCopyImageSubData(src_id, GL_TEXTURE_2D, 0, x, y, 0,
					dst_id, dst_type, 0, 0, 0, 0, width, height, 1);

				//Check for error
				if (GLenum err = glGetError())
				{
					LOG_WARNING(RSX, "Failed to copy image subresource with GL error 0x%X", err);
					return 0;
				}
			}

			//TODO: Native texture views are needed here. It works because this routine is only called with rendertarget data
			if (ifmt != sized_internal_fmt)
			{
				err_once("GL format mismatch (data cast?). Sized ifmt=0x%X vs Src ifmt=0x%X", sized_internal_fmt, ifmt);
				//Apply base component map onto the new texture if a data cast has been done
				apply_component_mapping_flags(dst_type, gcm_format, rsx::texture_create_flags::default_component_order);
			}
			else
			{
				//Inherit the parent's default mapping. The caller should ensure the native order is set beforehand
				GLint src_remap[4];
				glBindTexture(GL_TEXTURE_2D, src_id);
				glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, (GLint*)&src_remap[0]);
				glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, (GLint*)&src_remap[1]);
				glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, (GLint*)&src_remap[2]);
				glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, (GLint*)&src_remap[3]);

				glBindTexture(dst_type, dst_id);
				glTexParameteriv(dst_type, GL_TEXTURE_SWIZZLE_RGBA, src_remap);
			}

			if (memcmp(remap.first.data(), rsx::default_remap_vector.first.data(), 4) ||
				memcmp(remap.second.data(), rsx::default_remap_vector.second.data(), 4))
				set_up_remap_vector(dst_id, dst_type, remap);

			return dst_id;
		}

		void apply_component_mapping_flags(GLenum target, u32 gcm_format, rsx::texture_create_flags flags)
		{
			//NOTE: Depth textures should always read RRRR
			switch (gcm_format)
			{
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			{
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_RED);
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_RED);
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_RED);
				return;
			}
			default:
				break;
			}

			switch (flags)
			{
			case rsx::texture_create_flags::default_component_order:
			{
				auto remap = gl::get_swizzle_remap(gcm_format);
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, remap[1]);
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, remap[2]);
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, remap[3]);
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, remap[0]);
				break;
			}
			case rsx::texture_create_flags::native_component_order:
			{
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_RED);
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
				break;
			}
			case rsx::texture_create_flags::swapped_native_component_order:
			{
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_ALPHA);
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_RED);
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_GREEN);
				glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_BLUE);
				break;
			}
			}
		}

		void set_up_remap_vector(u32 texture_id, GLenum texture_type, const texture_channel_remap_t& remap_vector)
		{
			std::array<GLenum, 4> swizzle_remap;
			glBindTexture(texture_type, texture_id);
			glGetTexParameteriv(texture_type, GL_TEXTURE_SWIZZLE_A, (GLint*)&swizzle_remap[0]);
			glGetTexParameteriv(texture_type, GL_TEXTURE_SWIZZLE_R, (GLint*)&swizzle_remap[1]);
			glGetTexParameteriv(texture_type, GL_TEXTURE_SWIZZLE_G, (GLint*)&swizzle_remap[2]);
			glGetTexParameteriv(texture_type, GL_TEXTURE_SWIZZLE_B, (GLint*)&swizzle_remap[3]);

			apply_swizzle_remap(texture_type, swizzle_remap, remap_vector);
		}

	protected:

		void free_texture_section(cached_texture_section& tex) override
		{
			tex.destroy();
		}

		u32 create_temporary_subresource_view(void*&, u32* src, u32 gcm_format, u16 x, u16 y, u16 w, u16 h,
				const texture_channel_remap_t& remap_vector) override
		{
			return create_temporary_subresource_impl(*src, GL_NONE, GL_TEXTURE_2D, gcm_format, x, y, w, h, remap_vector, true);
		}

		u32 create_temporary_subresource_view(void*&, gl::texture* src, u32 gcm_format, u16 x, u16 y, u16 w, u16 h,
				const texture_channel_remap_t& remap_vector) override
		{
			if (auto as_rtt = dynamic_cast<gl::render_target*>(src))
			{
				return create_temporary_subresource_impl(src->id(), (GLenum)as_rtt->get_compatible_internal_format(),
						GL_TEXTURE_2D, gcm_format, x, y, w, h, remap_vector, true);
			}
			else
			{
				return create_temporary_subresource_impl(src->id(), GL_NONE, GL_TEXTURE_2D, gcm_format, x, y, w, h,
						remap_vector, true);
			}
		}

		u32 generate_cubemap_from_images(void*&, u32 gcm_format, u16 size, const std::array<u32, 6>& sources, const texture_channel_remap_t& /*remap_vector*/) override
		{
			const GLenum ifmt = gl::get_sized_internal_format(gcm_format);
			GLuint dst_id = 0;

			glGenTextures(1, &dst_id);
			glBindTexture(GL_TEXTURE_CUBE_MAP, dst_id);
			glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, ifmt, size, size);

			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);

			//Empty GL_ERROR
			glGetError();

			for (int i = 0; i < 6; ++i)
			{
				if (sources[i] != 0)
				{
					glCopyImageSubData(sources[i], GL_TEXTURE_2D, 0, 0, 0, 0,
						dst_id, GL_TEXTURE_CUBE_MAP, 0, 0, 0, i, size, size, 1);
				}
			}

			m_temporary_surfaces.push_back(dst_id);

			if (GLenum err = glGetError())
			{
				LOG_WARNING(RSX, "Failed to copy image subresource with GL error 0x%X", err);
				return 0;
			}

			return dst_id;
		}

		u32 generate_atlas_from_images(void*&, u32 gcm_format, u16 width, u16 height, const std::vector<copy_region_descriptor>& sections_to_copy,
				const texture_channel_remap_t& remap_vector) override
		{
			auto result = create_temporary_subresource_impl(sections_to_copy.front().src, GL_NONE, GL_TEXTURE_2D, gcm_format, 0, 0, width, height, remap_vector, false);

			for (const auto &region : sections_to_copy)
			{
				glCopyImageSubData(region.src, GL_TEXTURE_2D, 0, region.src_x, region.src_y, 0,
					result, GL_TEXTURE_2D, 0, region.dst_x, region.dst_y, 0, region.w, region.h, 1);
			}

			return result;
		}

		void update_image_contents(void*&, u32 dst, u32 src, u16 width, u16 height) override
		{
			glCopyImageSubData(src, GL_TEXTURE_2D, 0, 0, 0, 0,
					dst, GL_TEXTURE_2D, 0, 0, 0, 0, width, height, 1);
		}

		cached_texture_section* create_new_texture(void*&, u32 rsx_address, u32 rsx_size, u16 width, u16 height, u16 depth, u16 mipmaps, u32 gcm_format,
				rsx::texture_upload_context context, rsx::texture_dimension_extended type, rsx::texture_create_flags flags,
				rsx::texture_colorspace colorspace, const texture_channel_remap_t& /*remap_vector*/) override
		{
			u32 vram_texture = gl::create_texture(gcm_format, width, height, depth, mipmaps, type, colorspace);
			bool depth_flag = false;

			switch (gcm_format)
			{
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH16:
				depth_flag = true;
				break;
			}

			auto target = gl::get_target(type);
			glBindTexture(target, vram_texture);
			apply_component_mapping_flags(target, gcm_format, flags);

			auto& cached = create_texture(vram_texture, rsx_address, rsx_size, width, height, depth, mipmaps);
			cached.set_dirty(false);
			cached.set_depth_flag(depth_flag);
			cached.set_view_flags(flags);
			cached.set_context(context);
			cached.set_sampler_status(rsx::texture_sampler_status::status_uninitialized);
			cached.set_image_type(type);

			if (context != rsx::texture_upload_context::blit_engine_dst)
			{
				cached.protect(utils::protection::ro);
			}
			else
			{
				//TODO: More tests on byte order
				//ARGB8+native+unswizzled is confirmed with Dark Souls II character preview
				switch (gcm_format)
				{
				case CELL_GCM_TEXTURE_A8R8G8B8:
				{
					bool bgra = (flags == rsx::texture_create_flags::native_component_order);
					cached.set_format(bgra ? gl::texture::format::bgra : gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8, false);
					break;
				}
				case CELL_GCM_TEXTURE_R5G6B5:
				{
					cached.set_format(gl::texture::format::rgb, gl::texture::type::ushort_5_6_5, true);
					break;
				}
				case CELL_GCM_TEXTURE_DEPTH24_D8:
				{
					cached.set_format(gl::texture::format::depth_stencil, gl::texture::type::uint_24_8, true);
					break;
				}
				case CELL_GCM_TEXTURE_DEPTH16:
				{
					cached.set_format(gl::texture::format::depth, gl::texture::type::ushort, true);
					break;
				}
				default:
					fmt::throw_exception("Unexpected gcm format 0x%X" HERE, gcm_format);
				}

				cached.make_flushable();
				cached.set_dimensions(width, height, depth, (rsx_size / height));
				cached.protect(utils::protection::no);
				no_access_range = cached.get_min_max(no_access_range);
			}

			update_cache_tag();
			return &cached;
		}

		cached_texture_section* upload_image_from_cpu(void*&, u32 rsx_address, u16 width, u16 height, u16 depth, u16 mipmaps, u16 pitch, u32 gcm_format,
			rsx::texture_upload_context context, const std::vector<rsx_subresource_layout>& subresource_layout, rsx::texture_dimension_extended type,
			rsx::texture_colorspace colorspace, bool swizzled, const texture_channel_remap_t& remap_vector) override
		{
			void* unused = nullptr;
			auto section = create_new_texture(unused, rsx_address, pitch * height, width, height, depth, mipmaps, gcm_format, context, type,
				rsx::texture_create_flags::default_component_order, colorspace, remap_vector);

			bool input_swizzled = swizzled;
			if (context == rsx::texture_upload_context::blit_engine_src)
			{
				//Swizzling is ignored for blit engine copy and emulated using remapping
				input_swizzled = false;
				section->set_sampler_status(rsx::texture_sampler_status::status_uninitialized);
			}
			else
			{
				//Generic upload - sampler status will be set on upload
				section->set_sampler_status(rsx::texture_sampler_status::status_ready);
			}

			gl::upload_texture(section->get_raw_texture(), rsx_address, gcm_format, width, height, depth, mipmaps,
					input_swizzled, type, subresource_layout, remap_vector, false, colorspace);
			return section;
		}

		void enforce_surface_creation_type(cached_texture_section& section, u32 gcm_format, rsx::texture_create_flags flags) override
		{
			if (flags == section.get_view_flags())
				return;

			glBindTexture(GL_TEXTURE_2D, section.get_raw_texture());
			apply_component_mapping_flags(GL_TEXTURE_2D, gcm_format, flags);

			section.set_view_flags(flags);
			section.set_sampler_status(rsx::texture_sampler_status::status_uninitialized);
		}

		void set_up_remap_vector(cached_texture_section& section, const texture_channel_remap_t& remap_vector) override
		{
			set_up_remap_vector(section.get_raw_texture(), GL_TEXTURE_2D, remap_vector);
			section.set_sampler_status(rsx::texture_sampler_status::status_ready);
		}

		void insert_texture_barrier(void*&, gl::texture*) override
		{
			auto &caps = gl::get_driver_caps();

			if (caps.ARB_texture_barrier_supported)
				glTextureBarrier();
			else if (caps.NV_texture_barrier_supported)
				glTextureBarrierNV();
		}

		bool render_target_format_is_compatible(gl::texture* tex, u32 gcm_format) override
		{
			if (auto as_rtt = dynamic_cast<gl::render_target*>(tex))
			{
				auto ifmt = as_rtt->get_compatible_internal_format();
				switch (gcm_format)
				{
				default:
					//TODO
					err_once("Format incompatibility detected, reporting failure to force data copy (GL_INTERNAL_FORMAT=0x%X, GCM_FORMAT=0x%X)", (u32)ifmt, gcm_format);
					return false;
				case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
					return (ifmt == gl::texture::internal_format::rgba16f);
				case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
					return (ifmt == gl::texture::internal_format::rgba32f);
				case CELL_GCM_TEXTURE_X32_FLOAT:
					return (ifmt == gl::texture::internal_format::r32f);
				case CELL_GCM_TEXTURE_R5G6B5:
					return (ifmt == gl::texture::internal_format::r5g6b5);
				case CELL_GCM_TEXTURE_A8R8G8B8:
					return (ifmt == gl::texture::internal_format::rgba8 ||
							ifmt == gl::texture::internal_format::depth24_stencil8 ||
							ifmt == gl::texture::internal_format::depth32f_stencil8);
				case CELL_GCM_TEXTURE_B8:
					return (ifmt == gl::texture::internal_format::r8);
				case CELL_GCM_TEXTURE_G8B8:
					return (ifmt == gl::texture::internal_format::rg8);
				case CELL_GCM_TEXTURE_DEPTH24_D8:
				case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
					return (ifmt == gl::texture::internal_format::depth24_stencil8 ||
							ifmt == gl::texture::internal_format::depth32f_stencil8 ||
							ifmt == gl::texture::internal_format::depth_stencil);
				case CELL_GCM_TEXTURE_DEPTH16:
				case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
					return (ifmt == gl::texture::internal_format::depth16 ||
							ifmt == gl::texture::internal_format::depth);
				}
			}

			fmt::throw_exception("Format comparison for non-rendertargets is not implemented" HERE);
		}

	public:

		texture_cache() {}

		~texture_cache() {}

		void initialize()
		{
			m_hw_blitter.init();
			g_hw_blitter = &m_hw_blitter;
		}

		void destroy() override
		{
			clear();
			g_hw_blitter = nullptr;
			m_hw_blitter.destroy();
		}

		bool is_depth_texture(u32 rsx_address, u32 rsx_size) override
		{
			reader_lock lock(m_cache_mutex);

			auto found = m_cache.find(get_block_address(rsx_address));
			if (found == m_cache.end())
				return false;

			if (found->second.valid_count == 0)
				return false;

			for (auto& tex : found->second.data)
			{
				if (tex.is_dirty())
					continue;

				if (!tex.overlaps(rsx_address, true))
					continue;

				if ((rsx_address + rsx_size - tex.get_section_base()) <= tex.get_section_size())
					return tex.is_depth_texture();
			}

			return false;
		}

		void on_frame_end() override
		{
			if (m_unreleased_texture_objects >= m_max_zombie_objects)
			{
				purge_dirty();
			}

			clear_temporary_subresources();
			m_temporary_subresource_cache.clear();
			reset_frame_statistics();
		}

		bool blit(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool linear_interpolate, gl_render_targets& m_rtts)
		{
			void* unused = nullptr;
			auto result = upload_scaled_image(src, dst, linear_interpolate, unused, m_rtts, m_hw_blitter);

			if (result.succeeded)
			{
				if (result.real_dst_size)
				{
					gl::texture::format fmt;
					if (!result.is_depth)
					{
						fmt = dst.format == rsx::blit_engine::transfer_destination_format::a8r8g8b8 ?
							gl::texture::format::bgra : gl::texture::format::rgba;
					}
					else
					{
						fmt = dst.format == rsx::blit_engine::transfer_destination_format::a8r8g8b8 ?
							gl::texture::format::depth_stencil : gl::texture::format::depth;
					}

					flush_if_cache_miss_likely(fmt, result.real_dst_address, result.real_dst_size);
				}

				return true;
			}

			return false;
		}
	};
}
