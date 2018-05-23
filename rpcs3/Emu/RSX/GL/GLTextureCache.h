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
	extern void copy_typeless(texture*, const texture*);
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

		void scale_image(const texture* src, texture* dst, areai src_rect, areai dst_rect, bool linear_interpolation,
				bool is_depth_copy, const rsx::typeless_xfer& xfer_info)
		{
			std::unique_ptr<texture> typeless_src;
			std::unique_ptr<texture> typeless_dst;
			u32 src_id = src->id();
			u32 dst_id = dst->id();

			if (xfer_info.src_is_typeless)
			{
				const auto internal_width = (u16)(src->width() * xfer_info.src_scaling_hint);
				const auto internal_fmt = get_sized_internal_format(xfer_info.src_gcm_format);
				typeless_src = std::make_unique<texture>(GL_TEXTURE_2D, internal_width, src->height(), 1, 1, internal_fmt);
				copy_typeless(typeless_src.get(), src);

				src_id = typeless_src->id();
				src_rect.x1 = (u16)(src_rect.x1 * xfer_info.src_scaling_hint);
				src_rect.x2 = (u16)(src_rect.x2 * xfer_info.src_scaling_hint);
			}

			if (xfer_info.dst_is_typeless)
			{
				const auto internal_width = (u16)(dst->width() * xfer_info.dst_scaling_hint);
				const auto internal_fmt = get_sized_internal_format(xfer_info.dst_gcm_format);
				typeless_dst = std::make_unique<texture>(GL_TEXTURE_2D, internal_width, dst->height(), 1, 1, internal_fmt);
				copy_typeless(typeless_dst.get(), dst);

				dst_id = typeless_dst->id();
				dst_rect.x1 = (u16)(dst_rect.x1 * xfer_info.dst_scaling_hint);
				dst_rect.x2 = (u16)(dst_rect.x2 * xfer_info.dst_scaling_hint);
			}

			s32 old_fbo = 0;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);

			filter interp = linear_interpolation ? filter::linear : filter::nearest;
			GLenum attachment = is_depth_copy ? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0;

			blit_src.bind();
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, src_id, 0);
			blit_src.check();

			blit_dst.bind();
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, dst_id, 0);
			blit_dst.check();

			GLboolean scissor_test_enabled = glIsEnabled(GL_SCISSOR_TEST);
			if (scissor_test_enabled)
				glDisable(GL_SCISSOR_TEST);

			blit_src.blit(blit_dst, src_rect, dst_rect, is_depth_copy ? buffers::depth : buffers::color, interp);

			if (xfer_info.dst_is_typeless)
			{
				//Transfer contents from typeless dst back to original dst
				copy_typeless(dst, typeless_dst.get());
			}

			blit_src.bind();
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, GL_NONE, 0);

			blit_dst.bind();
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, GL_NONE, 0);

			if (scissor_test_enabled)
				glEnable(GL_SCISSOR_TEST);

			glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
		}
	};

	class cached_texture_section : public rsx::cached_texture_section
	{
	private:
		fence m_fence;
		u32 pbo_id = 0;
		u32 pbo_size = 0;

		gl::texture* vram_texture = nullptr;

		std::unique_ptr<gl::texture_view> view;
		std::unique_ptr<gl::texture> managed_texture;
		std::unique_ptr<gl::texture> scaled_texture;

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
			glBindBuffer(GL_PIXEL_PACK_BUFFER, GL_NONE);

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

			vram_texture = nullptr;
			managed_texture.reset();
			view.reset();
		}

		void create(u16 w, u16 h, u16 depth, u16 mipmaps, gl::texture_view* _view,
				gl::texture* image, u32 rsx_pitch, bool read_only,
				gl::texture::format gl_format, gl::texture::type gl_type, bool swap_bytes)
		{
			if (read_only)
			{
				managed_texture.reset(image);
				view.reset(_view);

				aa_mode = rsx::surface_antialiasing::center_1_sample;
			}
			else
			{
				view.reset();
				managed_texture.reset();

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

			vram_texture = image;
			set_format(gl_format, gl_type, swap_bytes);
		}

		void create_read_only(gl::texture* image, gl::texture_view* _view, u32 width, u32 height, u32 depth, u32 mipmaps)
		{
			//Only to be used for ro memory, we dont care about most members, just dimensions and the vram texture handle
			this->width = width;
			this->height = height;
			this->depth = depth;
			this->mipmaps = mipmaps;

			managed_texture.reset(image);
			view.reset(_view);
			vram_texture = image;

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
			vram_texture = &source;
		}

		void copy_texture(bool=false)
		{
			if (!pbo_id)
			{
				init_buffer();
			}

			gl::texture* target_texture = vram_texture;
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

				auto ifmt = vram_texture->get_internal_format();
				src_area.x2 = vram_texture->width();
				src_area.y2 = vram_texture->height();

				if (src_area.x2 != dst_area.x2 || src_area.y2 != dst_area.y2)
				{
					if (scaled_texture)
					{
						auto sfmt = scaled_texture->get_internal_format();
						if (scaled_texture->width() != real_width ||
							scaled_texture->height() != real_height ||
							sfmt != ifmt)
						{
							//Discard current scaled texture
							scaled_texture.reset();
						}
					}

					if (!scaled_texture)
					{
						scaled_texture = std::make_unique<gl::texture>(GL_TEXTURE_2D, real_width, real_height, 1, 1, (GLenum)ifmt);
					}

					bool linear_interp = false; //TODO: Make optional or detect full sized sources
					g_hw_blitter->scale_image(vram_texture, scaled_texture.get(), src_area, dst_area, linear_interp, is_depth, {});
					target_texture = scaled_texture.get();
				}
			}

			glGetError();
			glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_id);

			pixel_pack_settings pack_settings;
			pack_settings.alignment(1);

			//NOTE: AMD proprietary driver bug - disable swap bytes
			if (!::gl::get_driver_caps().vendor_AMD)
				pack_settings.swap_bytes(pack_unpack_swap_bytes);

			target_texture->copy_to(nullptr, format, type, pack_settings);

			if (auto error = glGetError())
			{
				if (error == GL_OUT_OF_MEMORY && ::gl::get_driver_caps().vendor_AMD)
				{
					//AMD driver bug
					//Pixel transfer fails with GL_OUT_OF_MEMORY. Usually happens with float textures
					//Failed operations also leak a large amount of memory
					LOG_ERROR(RSX, "Memory transfer failure (AMD bug). Format=0x%x, Type=0x%x", (u32)format, (u32)type);
				}
				else
				{
					LOG_ERROR(RSX, "Memory transfer failed with error 0x%x. Format=0x%x, Type=0x%x", error, (u32)format, (u32)type);
				}
			}

			glBindBuffer(GL_PIXEL_PACK_BUFFER, GL_NONE);

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

			glBindTexture(GL_TEXTURE_2D, tex->id());
			glPixelStorei(GL_UNPACK_SWAP_BYTES, pack_unpack_swap_bytes);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_id);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, min_width, min_height, (GLenum)format, (GLenum)type, nullptr);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
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

			const auto valid_range = get_confirmed_range();
			void *dst = get_raw_ptr(valid_range.first, true);

			glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_id);
			void *src = glMapBufferRange(GL_PIXEL_PACK_BUFFER, valid_range.first, valid_range.second, GL_MAP_READ_BIT);

			//throw if map failed since we'll segfault anyway
			verify(HERE), src != nullptr;

			bool require_manual_shuffle = false;
			if (pack_unpack_swap_bytes)
			{
				if (type == gl::texture::type::sbyte || type == gl::texture::type::ubyte)
					require_manual_shuffle = true;
			}

			if (real_pitch >= rsx_pitch || scaled_texture != 0)
			{
				memcpy(dst, src, valid_range.second);
			}
			else
			{
				fmt::throw_exception("Unreachable");
			}

			if (require_manual_shuffle)
			{
				//byte swapping does not work on byte types, use uint_8_8_8_8 for rgba8 instead to avoid penalty
				rsx::shuffle_texel_data_wzyx<u8>(dst, rsx_pitch, width, valid_range.second / rsx_pitch);
			}
			else if (pack_unpack_swap_bytes && ::gl::get_driver_caps().vendor_AMD)
			{
				//AMD driver bug - cannot use pack_swap_bytes
				//Manually byteswap texel data
				switch (type)
				{
				case texture::type::f16:
				case texture::type::sshort:
				case texture::type::ushort:
				case texture::type::ushort_5_6_5:
				case texture::type::ushort_4_4_4_4:
				case texture::type::ushort_1_5_5_5_rev:
				case texture::type::ushort_5_5_5_1:
				{
					const u32 num_reps = valid_range.second / 2;
					be_t<u16>* in = (be_t<u16>*)(dst);
					u16* out = (u16*)dst;

					for (u32 n = 0; n < num_reps; ++n)
					{
						out[n] = in[n];
					}

					break;
				}
				case texture::type::f32:
				case texture::type::sint:
				case texture::type::uint:
				case texture::type::uint_10_10_10_2:
				case texture::type::uint_24_8:
				case texture::type::uint_2_10_10_10_rev:
				case texture::type::uint_8_8_8_8:
				{
					u32 num_reps = valid_range.second / 4;
					be_t<u32>* in = (be_t<u32>*)(dst);
					u32* out = (u32*)dst;

					for (u32 n = 0; n < num_reps; ++n)
					{
						out[n] = in[n];
					}

					break;
				}
				default:
				{
					LOG_ERROR(RSX, "Texture type 0x%x is not implemented " HERE, (u32)type);
					break;
				}
				}
			}

			flush_io(valid_range.first, valid_range.second);
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, GL_NONE);

			reset_write_statistics();

			return result;
		}

		void reprotect(utils::protection prot, const std::pair<u32, u32>& range)
		{
			flushed = false;
			synchronized = false;
			protect(prot, range);
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
				managed_texture.reset();
				view.reset();
			}
			else
			{
				//Destroy pbo cache since vram texture is managed elsewhere
				glDeleteBuffers(1, &pbo_id);
				scaled_texture.reset();
			}

			vram_texture = nullptr;
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

		gl::texture_view* get_raw_view() const
		{
			return view.get();
		}

		gl::texture* get_raw_texture() const
		{
			return managed_texture.get();
		}

		std::unique_ptr<gl::texture_view>& get_view()
		{
			return view;
		}

		bool is_depth_texture() const
		{
			return is_depth;
		}

		bool has_compatible_format(gl::texture* tex) const
		{
			//TODO
			return (tex->get_internal_format() == vram_texture->get_internal_format());
		}
	};

	class texture_cache : public rsx::texture_cache<void*, cached_texture_section, gl::texture*, gl::texture_view*, gl::texture, gl::texture::format>
	{
	private:

		struct discardable_storage
		{
			std::unique_ptr<gl::texture> image;
			std::unique_ptr<gl::texture_view> view;

			discardable_storage()
			{}

			discardable_storage(std::unique_ptr<gl::texture>& tex)
			{
				image = std::move(tex);
			}

			discardable_storage(std::unique_ptr<gl::texture_view>& _view)
			{
				view = std::move(_view);
			}

			discardable_storage(std::unique_ptr<gl::texture>& tex, std::unique_ptr<gl::texture_view>& _view)
			{
				image = std::move(tex);
				view = std::move(_view);
			}
		};

	private:

		blitter m_hw_blitter;
		std::vector<discardable_storage> m_temporary_surfaces;

		cached_texture_section& create_texture(gl::texture* image, gl::texture_view* view, u32 texaddr, u32 texsize, u32 w, u32 h, u32 depth, u32 mipmaps)
		{
			cached_texture_section& tex = find_cached_texture(texaddr, texsize, true, w, h, depth);
			tex.reset(texaddr, texsize, false);
			tex.create_read_only(image, view, w, h, depth, mipmaps);
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
			m_temporary_surfaces.resize(0);
		}

		gl::texture_view* create_temporary_subresource_impl(gl::texture* src, GLenum sized_internal_fmt, GLenum dst_type, u32 gcm_format,
				u16 x, u16 y, u16 width, u16 height, const texture_channel_remap_t& remap, bool copy)
		{
			if (sized_internal_fmt == GL_NONE)
				sized_internal_fmt = gl::get_sized_internal_format(gcm_format);

			gl::texture::internal_format ifmt = static_cast<gl::texture::internal_format>(sized_internal_fmt);
			if (src)
			{
				ifmt = src->get_internal_format();
				switch (ifmt)
				{
				case gl::texture::internal_format::depth16:
				case gl::texture::internal_format::depth24_stencil8:
				case gl::texture::internal_format::depth32f_stencil8:
					//HACK! Should use typeless transfer instead
					sized_internal_fmt = (GLenum)ifmt;
					break;
				}
			}

			auto dst = std::make_unique<gl::texture>(dst_type, width, height, 1, 1, sized_internal_fmt);

			if (copy)
			{
				//Empty GL_ERROR
				glGetError();

				glCopyImageSubData(src->id(), GL_TEXTURE_2D, 0, x, y, 0,
					dst->id(), dst_type, 0, 0, 0, 0, width, height, 1);

				//Check for error
				if (GLenum err = glGetError())
				{
					LOG_WARNING(RSX, "Failed to copy image subresource with GL error 0x%X", err);
					return nullptr;
				}
			}

			std::array<GLenum, 4> swizzle;
			if (!src || (GLenum)ifmt != sized_internal_fmt)
			{
				if (src)
				{
					//Format mismatch
					err_once("GL format mismatch (data cast?). Sized ifmt=0x%X vs Src ifmt=0x%X", sized_internal_fmt, (GLenum)ifmt);
				}

				//Apply base component map onto the new texture if a data cast has been done
				swizzle = get_component_mapping(gcm_format, rsx::texture_create_flags::default_component_order);
			}
			else
			{
				swizzle = src->get_native_component_layout();
			}

			if (memcmp(remap.first.data(), rsx::default_remap_vector.first.data(), 4) ||
				memcmp(remap.second.data(), rsx::default_remap_vector.second.data(), 4))
				swizzle = apply_swizzle_remap(swizzle, remap);

			auto view = std::make_unique<gl::texture_view>(dst.get(), dst_type, sized_internal_fmt, swizzle.data());
			auto result = view.get();

			m_temporary_surfaces.push_back({ dst, view });
			return result;
		}

		std::array<GLenum, 4> get_component_mapping(u32 gcm_format, rsx::texture_create_flags flags)
		{
			switch (gcm_format)
			{
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
				//Dont bother letting this propagate
				return{ GL_RED, GL_RED, GL_RED, GL_RED };
			default:
				break;
			}

			switch (flags)
			{
			case rsx::texture_create_flags::default_component_order:
			{
				return gl::get_swizzle_remap(gcm_format);
			}
			case rsx::texture_create_flags::native_component_order:
			{
				return{ GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE };
			}
			case rsx::texture_create_flags::swapped_native_component_order:
			{
				return{ GL_BLUE, GL_ALPHA, GL_RED, GL_GREEN };
			}
			default:
				fmt::throw_exception("Unknown texture create flags" HERE);
			}
		}

	protected:

		void free_texture_section(cached_texture_section& tex) override
		{
			tex.destroy();
		}

		gl::texture_view* create_temporary_subresource_view(void*&, gl::texture** src, u32 gcm_format, u16 x, u16 y, u16 w, u16 h,
				const texture_channel_remap_t& remap_vector) override
		{
			return create_temporary_subresource_impl(*src, GL_NONE, GL_TEXTURE_2D, gcm_format, x, y, w, h, remap_vector, true);
		}

		gl::texture_view* create_temporary_subresource_view(void*&, gl::texture* src, u32 gcm_format, u16 x, u16 y, u16 w, u16 h,
				const texture_channel_remap_t& remap_vector) override
		{
			return create_temporary_subresource_impl(src, (GLenum)src->get_internal_format(),
					GL_TEXTURE_2D, gcm_format, x, y, w, h, remap_vector, true);
		}

		gl::texture_view* generate_cubemap_from_images(void*&, u32 gcm_format, u16 size, const std::vector<copy_region_descriptor>& sources, const texture_channel_remap_t& /*remap_vector*/) override
		{
			const GLenum ifmt = gl::get_sized_internal_format(gcm_format);
			auto dst_image = std::make_unique<gl::texture>(GL_TEXTURE_CUBE_MAP, size, size, 1, 1, ifmt);
			auto view = std::make_unique<gl::texture_view>(dst_image.get(), GL_TEXTURE_CUBE_MAP, ifmt);

			//Empty GL_ERROR
			glGetError();

			for (const auto &slice : sources)
			{
				if (slice.src)
				{
					glCopyImageSubData(slice.src->id(), GL_TEXTURE_2D, 0, slice.src_x, slice.src_y, 0,
						dst_image->id(), GL_TEXTURE_CUBE_MAP, 0, slice.dst_x, slice.dst_y, slice.dst_z, slice.w, slice.h, 1);
				}
			}

			if (GLenum err = glGetError())
			{
				LOG_WARNING(RSX, "Failed to copy image subresource with GL error 0x%X", err);
				return nullptr;
			}

			auto result = view.get();
			m_temporary_surfaces.push_back({ dst_image, view });
			return result;
		}

		gl::texture_view* generate_3d_from_2d_images(void*&, u32 gcm_format, u16 width, u16 height, u16 depth, const std::vector<copy_region_descriptor>& sources, const texture_channel_remap_t& /*remap_vector*/) override
		{
			const GLenum ifmt = gl::get_sized_internal_format(gcm_format);
			auto dst_image = std::make_unique<gl::texture>(GL_TEXTURE_3D, width, height, depth, 1, ifmt);
			auto view = std::make_unique<gl::texture_view>(dst_image.get(), GL_TEXTURE_3D, ifmt);

			//Empty GL_ERROR
			glGetError();

			for (const auto &slice : sources)
			{
				if (slice.src)
				{
					glCopyImageSubData(slice.src->id(), GL_TEXTURE_2D, 0, slice.src_x, slice.src_y, 0,
						dst_image->id(), GL_TEXTURE_3D, 0, slice.dst_x, slice.dst_y, slice.dst_z, slice.w, slice.h, 1);
				}
			}

			if (GLenum err = glGetError())
			{
				LOG_WARNING(RSX, "Failed to copy image subresource with GL error 0x%X", err);
				return nullptr;
			}

			auto result = view.get();
			m_temporary_surfaces.push_back({ dst_image, view });
			return result;
		}

		gl::texture_view* generate_atlas_from_images(void*&, u32 gcm_format, u16 width, u16 height, const std::vector<copy_region_descriptor>& sections_to_copy,
				const texture_channel_remap_t& remap_vector) override
		{
			auto result = create_temporary_subresource_impl(nullptr, GL_NONE, GL_TEXTURE_2D, gcm_format, 0, 0, width, height, remap_vector, false);

			for (const auto &region : sections_to_copy)
			{
				glCopyImageSubData(region.src->id(), GL_TEXTURE_2D, 0, region.src_x, region.src_y, 0,
					result->image()->id(), GL_TEXTURE_2D, 0, region.dst_x, region.dst_y, 0, region.w, region.h, 1);
			}

			return result;
		}

		void update_image_contents(void*&, gl::texture_view* dst, gl::texture* src, u16 width, u16 height) override
		{
			glCopyImageSubData(src->id(), GL_TEXTURE_2D, 0, 0, 0, 0,
					dst->image()->id(), GL_TEXTURE_2D, 0, 0, 0, 0, width, height, 1);
		}

		cached_texture_section* create_new_texture(void*&, u32 rsx_address, u32 rsx_size, u16 width, u16 height, u16 depth, u16 mipmaps, u32 gcm_format,
				rsx::texture_upload_context context, rsx::texture_dimension_extended type, rsx::texture_create_flags flags,
				rsx::texture_colorspace colorspace, const texture_channel_remap_t& remap_vector) override
		{
			bool depth_flag = false;
			switch (gcm_format)
			{
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH16:
				depth_flag = true;
				break;
			}

			auto image = gl::create_texture(gcm_format, width, height, depth, mipmaps, type, colorspace);
			auto swizzle = get_component_mapping(gcm_format, flags);
			swizzle = gl::apply_swizzle_remap(swizzle, remap_vector);
			auto view = new gl::texture_view(image, swizzle.data());

			auto& cached = create_texture(image, view, rsx_address, rsx_size, width, height, depth, mipmaps);
			cached.set_dirty(false);
			cached.set_depth_flag(depth_flag);
			cached.set_view_flags(flags);
			cached.set_context(context);
			cached.set_gcm_format(gcm_format);
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

				//NOTE: Protection is handled by the caller
				cached.make_flushable();
				cached.set_dimensions(width, height, depth, (rsx_size / height));
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

			gl::upload_texture(section->get_raw_texture()->id(), rsx_address, gcm_format, width, height, depth, mipmaps,
					input_swizzled, type, subresource_layout, remap_vector, false, colorspace);
			return section;
		}

		void enforce_surface_creation_type(cached_texture_section& section, u32 gcm_format, rsx::texture_create_flags flags) override
		{
			if (flags == section.get_view_flags())
				return;

			auto swizzle = get_component_mapping(gcm_format, flags);
			auto& view = section.get_view();

			if (!view->compare_swizzle(swizzle.data()))
			{
				view.reset(new gl::texture_view(view->image(), swizzle.data()));
			}

			section.set_view_flags(flags);
			section.set_sampler_status(rsx::texture_sampler_status::status_uninitialized);
		}

		void set_up_remap_vector(cached_texture_section& section, const texture_channel_remap_t& remap_vector) override
		{
			auto& view = section.get_view();
			auto swizzle = view->component_mapping();

			swizzle = apply_swizzle_remap(swizzle, remap_vector);
			if (!view->compare_swizzle(swizzle.data()))
			{
				view.reset(new gl::texture_view(view->image(), swizzle.data()));
			}

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
			auto ifmt = tex->get_internal_format();
			switch (gcm_format)
			{
			default:
				//TODO
				warn_once("Format incompatibility detected, reporting failure to force data copy (GL_INTERNAL_FORMAT=0x%X, GCM_FORMAT=0x%X)", (u32)ifmt, gcm_format);
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

				if (!tex.overlaps(rsx_address, rsx::overlap_test_bounds::full_range))
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
