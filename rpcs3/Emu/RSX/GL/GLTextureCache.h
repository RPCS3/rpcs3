#pragma once

#include "Emu/RSX/GL/GLTexture.h"
#include "GLRenderTargets.h"
#include "glutils/blitter.h"
#include "glutils/sync.hpp"

#include "../Common/texture_cache.h"

#include <memory>
#include <vector>

class GLGSRender;

namespace gl
{
	class cached_texture_section;
	class texture_cache;

	struct texture_cache_traits
	{
		using commandbuffer_type      = gl::command_context;
		using section_storage_type    = gl::cached_texture_section;
		using texture_cache_type      = gl::texture_cache;
		using texture_cache_base_type = rsx::texture_cache<texture_cache_type, texture_cache_traits>;
		using image_resource_type     = gl::texture*;
		using image_view_type         = gl::texture_view*;
		using image_storage_type      = gl::texture;
		using texture_format          = gl::texture::format;
		using viewable_image_type     = gl::viewable_image*;
	};

	class cached_texture_section : public rsx::cached_texture_section<gl::cached_texture_section, gl::texture_cache_traits>
	{
		using baseclass = rsx::cached_texture_section<gl::cached_texture_section, gl::texture_cache_traits>;
		friend baseclass;

		fence m_fence;
		buffer pbo;

		gl::viewable_image* vram_texture = nullptr;

		std::unique_ptr<gl::viewable_image> managed_texture;
		std::unique_ptr<gl::texture> scaled_texture;

		texture::format format = texture::format::rgba;
		texture::type type = texture::type::ubyte;

		void init_buffer(const gl::texture* src)
		{
			const u32 vram_size = src->pitch() * src->height();
			const u32 buffer_size = utils::align(vram_size, 4096);

			if (pbo)
			{
				if (pbo.size() >= buffer_size)
					return;

				pbo.remove();
			}

			pbo.create(buffer::target::pixel_pack, buffer_size, nullptr, buffer::memory_type::host_visible, buffer::usage::host_read);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, GL_NONE);
		}

	public:
		using baseclass::cached_texture_section;

		void create(u16 w, u16 h, u16 depth, u16 mipmaps, gl::texture* image, u32 rsx_pitch, bool managed,
				gl::texture::format gl_format = gl::texture::format::rgba, gl::texture::type gl_type = gl::texture::type::ubyte, bool swap_bytes = false)
		{
			auto new_texture = static_cast<gl::viewable_image*>(image);
			ensure(!exists() || !is_managed() || vram_texture == new_texture);

			if (vram_texture != new_texture && !managed_texture && get_protection() == utils::protection::no)
			{
				// In-place image swap, still locked. Likely a color buffer that got rebound as depth buffer or vice-versa.
				gl::as_rtt(vram_texture)->on_swap_out();

				if (!managed)
				{
					// Incoming is also an external resource, reference it immediately
					gl::as_rtt(image)->on_swap_in(is_locked());
				}
			}

			vram_texture = new_texture;

			if (managed)
			{
				managed_texture.reset(vram_texture);
			}
			else
			{
				ensure(!managed_texture);
			}

			if (auto rtt = dynamic_cast<gl::render_target*>(image))
			{
				swizzled = (rtt->raster_type != rsx::surface_raster_type::linear);
			}

			flushed = false;
			synchronized = false;
			sync_timestamp = 0ull;

			ensure(rsx_pitch);

			this->rsx_pitch = rsx_pitch;
			this->width = w;
			this->height = h;
			this->real_pitch = 0;
			this->depth = depth;
			this->mipmaps = mipmaps;

			set_format(gl_format, gl_type, swap_bytes);

			// Notify baseclass
			baseclass::on_section_resources_created();
		}

		void set_dimensions(u32 width, u32 height, u32 /*depth*/, u32 pitch)
		{
			this->width = width;
			this->height = height;
			rsx_pitch = pitch;
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
				default:
					break;
				}
			}
		}

		void dma_transfer(gl::command_context& cmd, gl::texture* src, const areai& /*src_area*/, const utils::address_range32& /*valid_range*/, u32 pitch)
		{
			init_buffer(src);
			glGetError();

			if (context == rsx::texture_upload_context::dma)
			{
				// Determine unpack config dynamically
				const auto format_info = gl::get_format_type(src->get_internal_format());
				format = static_cast<gl::texture::format>(format_info.format);
				type = static_cast<gl::texture::type>(format_info.type);
				pack_unpack_swap_bytes = format_info.swap_bytes;
			}

			real_pitch = src->pitch();
			rsx_pitch = pitch;

			bool use_driver_pixel_transform = true;
			if (get_driver_caps().ARB_compute_shader_supported) [[likely]]
			{
				if (src->aspect() & image_aspect::depth)
				{
					buffer scratch_mem;

					// Invoke compute
					if (auto error = glGetError(); !error) [[likely]]
					{
						pixel_buffer_layout pack_info{};
						image_memory_requirements mem_info{};

						pack_info.format = static_cast<GLenum>(format);
						pack_info.type = static_cast<GLenum>(type);
						pack_info.size = (src->aspect() & image_aspect::stencil) ? 4 : 2;
						pack_info.swap_bytes = true;

						mem_info.image_size_in_texels = src->width() * src->height();
						mem_info.image_size_in_bytes = src->pitch() * src->height();
						mem_info.memory_required = 0;

						if (pack_info.type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV)
						{
							// D32FS8 can be read back as D24S8 or D32S8X24. In case of the latter, double memory requirements
							mem_info.image_size_in_bytes *= 2;
						}

						void* out_offset = copy_image_to_buffer(cmd, pack_info, src, &scratch_mem, 0, 0, { {}, src->size3D() }, &mem_info);

						glBindBuffer(GL_SHADER_STORAGE_BUFFER, GL_NONE);
						glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

						real_pitch = pack_info.size * src->width();
						const u64 data_length = pack_info.size * mem_info.image_size_in_texels;
						scratch_mem.copy_to(&pbo, reinterpret_cast<u64>(out_offset), 0, data_length);
					}
					else
					{
						rsx_log.error("Memory transfer failed with error 0x%x. Format=0x%x, Type=0x%x", error, static_cast<u32>(format), static_cast<u32>(type));
					}

					scratch_mem.remove();
					use_driver_pixel_transform = false;
				}
			}

			if (use_driver_pixel_transform)
			{
				if (src->aspect() & image_aspect::stencil)
				{
					pack_unpack_swap_bytes = false;
				}

				pbo.bind(buffer::target::pixel_pack);

				pixel_pack_settings pack_settings;
				pack_settings.alignment(1);
				pack_settings.swap_bytes(pack_unpack_swap_bytes);

				src->copy_to(nullptr, format, type, pack_settings);
			}

			if (auto error = glGetError())
			{
				if (error == GL_OUT_OF_MEMORY && ::gl::get_driver_caps().vendor_AMD)
				{
					// AMD driver bug
					// Pixel transfer fails with GL_OUT_OF_MEMORY. Usually happens with float textures or operations attempting to swap endianness.
					// Failed operations also leak a large amount of memory
					rsx_log.error("Memory transfer failure (AMD bug). Please update your driver to Adrenalin 19.4.3 or newer. Format=0x%x, Type=0x%x, Swap=%d", static_cast<u32>(format), static_cast<u32>(type), pack_unpack_swap_bytes);
				}
				else
				{
					rsx_log.error("Memory transfer failed with error 0x%x. Format=0x%x, Type=0x%x", error, static_cast<u32>(format), static_cast<u32>(type));
				}
			}

			glBindBuffer(GL_PIXEL_PACK_BUFFER, GL_NONE);

			m_fence.reset();
			synchronized = true;
			sync_timestamp = rsx::get_shared_tag();
		}

		void copy_texture(gl::command_context& cmd, bool miss)
		{
			ensure(exists());

			if (!miss) [[likely]]
			{
				baseclass::on_speculative_flush();
			}
			else
			{
				baseclass::on_miss();
			}

			gl::texture* target_texture = vram_texture;
			u32 transfer_width = width;
			u32 transfer_height = height;

			if (context == rsx::texture_upload_context::framebuffer_storage)
			{
				auto surface = gl::as_rtt(vram_texture);
				surface->memory_barrier(cmd, rsx::surface_access::transfer_read);
				target_texture = surface->get_surface(rsx::surface_access::transfer_read);
				transfer_width *= surface->samples_x;
				transfer_height *= surface->samples_y;
			}

			if ((rsx::get_resolution_scale_percent() != 100 && context == rsx::texture_upload_context::framebuffer_storage) ||
				(vram_texture->pitch() != rsx_pitch))
			{
				areai src_area = { 0, 0, 0, 0 };
				const areai dst_area = { 0, 0, static_cast<s32>(transfer_width), static_cast<s32>(transfer_height) };

				auto ifmt = vram_texture->get_internal_format();
				src_area.x2 = vram_texture->width();
				src_area.y2 = vram_texture->height();

				if (src_area.x2 != dst_area.x2 || src_area.y2 != dst_area.y2)
				{
					if (scaled_texture)
					{
						auto sfmt = scaled_texture->get_internal_format();
						if (scaled_texture->width() != transfer_width ||
							scaled_texture->height() != transfer_height ||
							sfmt != ifmt)
						{
							// Discard current scaled texture
							scaled_texture.reset();
						}
					}

					if (!scaled_texture)
					{
						scaled_texture = std::make_unique<gl::texture>(GL_TEXTURE_2D, transfer_width, transfer_height, 1, 1, 1, static_cast<GLenum>(ifmt), vram_texture->format_class());
					}

					const bool linear_interp = is_depth_texture() ? false : true;
					g_hw_blitter->scale_image(cmd, target_texture, scaled_texture.get(), src_area, dst_area, linear_interp, {});
					target_texture = scaled_texture.get();
				}
			}

			dma_transfer(cmd, target_texture, {}, {}, rsx_pitch);
		}

		/**
		 * Flush
		 */
		void* map_synchronized(u32 offset, u32 size)
		{
			AUDIT(synchronized && !m_fence.is_empty());

			m_fence.wait_for_signal();

			ensure(offset + GLsizeiptr{size} <= pbo.size());
			return pbo.map(offset, size, gl::buffer::access::read);
		}

		void finish_flush();

		/**
		 * Misc
		 */
		void destroy()
		{
			if (!is_locked() && !pbo && vram_texture == nullptr && m_fence.is_empty() && !managed_texture)
				//Already destroyed
				return;

			if (pbo)
			{
				// Destroy pbo cache since vram texture is managed elsewhere
				pbo.remove();
				scaled_texture.reset();
			}

			managed_texture.reset();
			vram_texture = nullptr;

			if (!m_fence.is_empty())
			{
				m_fence.destroy();
			}

			baseclass::on_section_resources_destroyed();
		}

		void sync_surface_memory(const std::vector<cached_texture_section*>& surfaces)
		{
			auto rtt = gl::as_rtt(vram_texture);
			rtt->sync_tag();

			for (auto& surface : surfaces)
			{
				rtt->inherit_surface_contents(gl::as_rtt(surface->vram_texture));
			}
		}

		bool exists() const
		{
			return (vram_texture != nullptr);
		}

		bool is_managed() const
		{
			return !exists() || managed_texture;
		}

		texture::format get_format() const
		{
			return format;
		}

		gl::texture_view* get_view(const rsx::texture_channel_remap_t& remap)
		{
			return vram_texture->get_view(remap);
		}

		gl::viewable_image* get_raw_texture() const
		{
			return managed_texture.get();
		}

		gl::render_target* get_render_target() const
		{
			return gl::as_rtt(vram_texture);
		}

		gl::texture_view* get_raw_view()
		{
			return vram_texture->get_view(rsx::default_remap_vector.with_encoding(GL_REMAP_IDENTITY));
		}

		bool is_depth_texture() const
		{
			return !!(vram_texture->aspect() & gl::image_aspect::depth);
		}

		bool has_compatible_format(gl::texture* tex) const
		{
			//TODO
			return (tex->get_internal_format() == vram_texture->get_internal_format());
		}
	};

	class texture_cache : public rsx::texture_cache<gl::texture_cache, gl::texture_cache_traits>
	{
	private:
		using baseclass = rsx::texture_cache<gl::texture_cache, gl::texture_cache_traits>;
		friend baseclass;

		struct temporary_image_t : public gl::viewable_image, public rsx::ref_counted
		{
			u64 properties_encoding = 0;

			using gl::viewable_image::viewable_image;
		};

		blitter m_hw_blitter;
		std::vector<std::unique_ptr<temporary_image_t>> m_temporary_surfaces;

		const u32 max_cached_image_pool_size = 256;

	private:
		void clear()
		{
			baseclass::clear();
			clear_temporary_subresources();
		}

		void clear_temporary_subresources()
		{
			m_temporary_surfaces.clear();
		}

		gl::texture_view* create_temporary_subresource_impl(gl::command_context& cmd, gl::texture* src, GLenum sized_internal_fmt, GLenum dst_type, u32 gcm_format,
				u16 x, u16 y, u16 width, u16 height, u16 depth, u8 mipmaps, const rsx::texture_channel_remap_t& remap, bool copy);

		std::array<GLenum, 4> get_component_mapping(u32 gcm_format, rsx::component_order flags) const
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
			case rsx::component_order::default_:
			{
				return gl::get_swizzle_remap(gcm_format);
			}
			case rsx::component_order::native:
			{
				return{ GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE };
			}
			case rsx::component_order::swapped_native:
			{
				return{ GL_BLUE, GL_ALPHA, GL_RED, GL_GREEN };
			}
			default:
				fmt::throw_exception("Unknown texture create flags");
			}
		}

		void copy_transfer_regions_impl(gl::command_context& cmd, gl::texture* dst_image, const std::vector<copy_region_descriptor>& sources) const;

		gl::texture* get_template_from_collection_impl(const std::vector<copy_region_descriptor>& sections_to_transfer) const
		{
			if (sections_to_transfer.size() == 1) [[likely]]
			{
				return sections_to_transfer.front().src;
			}

			gl::texture* result = nullptr;
			for (const auto &section : sections_to_transfer)
			{
				if (!section.src)
					continue;

				if (!result)
				{
					result = section.src;
				}
				else
				{
					const auto set1 = result->get_native_component_layout();
					const auto set2 = section.src->get_native_component_layout();

					if (set1[0] != set2[0] ||
						set1[1] != set2[1] ||
						set1[2] != set2[2] ||
						set1[3] != set2[3])
					{
						// TODO
						// This requires a far more complex setup as its not always possible to mix and match without compute assistance
						return nullptr;
					}
				}
			}

			return result;
		}

	protected:

		gl::texture_view* create_temporary_subresource_view(gl::command_context& cmd, gl::texture** src, u32 gcm_format, u16 x, u16 y, u16 w, u16 h,
				const rsx::texture_channel_remap_t& remap_vector) override
		{
			return create_temporary_subresource_impl(cmd, *src, GL_NONE, GL_TEXTURE_2D, gcm_format, x, y, w, h, 1, 1, remap_vector, true);
		}

		gl::texture_view* create_temporary_subresource_view(gl::command_context& cmd, gl::texture* src, u32 gcm_format, u16 x, u16 y, u16 w, u16 h,
				const rsx::texture_channel_remap_t& remap_vector) override
		{
			return create_temporary_subresource_impl(cmd, src, static_cast<GLenum>(src->get_internal_format()),
					GL_TEXTURE_2D, gcm_format, x, y, w, h, 1, 1, remap_vector, true);
		}

		gl::texture_view* generate_cubemap_from_images(gl::command_context& cmd, u32 gcm_format, u16 size, const std::vector<copy_region_descriptor>& sources, const rsx::texture_channel_remap_t& remap_vector) override
		{
			auto _template = get_template_from_collection_impl(sources);
			auto result = create_temporary_subresource_impl(cmd, _template, GL_NONE, GL_TEXTURE_CUBE_MAP, gcm_format, 0, 0, size, size, 1, 1, remap_vector, false);

			copy_transfer_regions_impl(cmd, result->image(), sources);
			return result;
		}

		gl::texture_view* generate_3d_from_2d_images(gl::command_context& cmd, u32 gcm_format, u16 width, u16 height, u16 depth, const std::vector<copy_region_descriptor>& sources, const rsx::texture_channel_remap_t& remap_vector) override
		{
			auto _template = get_template_from_collection_impl(sources);
			auto result = create_temporary_subresource_impl(cmd, _template, GL_NONE, GL_TEXTURE_3D, gcm_format, 0, 0, width, height, depth, 1, remap_vector, false);

			copy_transfer_regions_impl(cmd, result->image(), sources);
			return result;
		}

		gl::texture_view* generate_atlas_from_images(gl::command_context& cmd, u32 gcm_format, u16 width, u16 height, const std::vector<copy_region_descriptor>& sections_to_copy,
				const rsx::texture_channel_remap_t& remap_vector) override
		{
			auto _template = get_template_from_collection_impl(sections_to_copy);
			auto result = create_temporary_subresource_impl(cmd, _template, GL_NONE, GL_TEXTURE_2D, gcm_format, 0, 0, width, height, 1, 1, remap_vector, false);

			copy_transfer_regions_impl(cmd, result->image(), sections_to_copy);
			return result;
		}

		gl::texture_view* generate_2d_mipmaps_from_images(gl::command_context& cmd, u32 gcm_format, u16 width, u16 height, const std::vector<copy_region_descriptor>& sections_to_copy,
			const rsx::texture_channel_remap_t& remap_vector) override
		{
			const auto mipmaps = ::narrow<u8>(sections_to_copy.size());
			auto _template = get_template_from_collection_impl(sections_to_copy);
			auto result = create_temporary_subresource_impl(cmd, _template, GL_NONE, GL_TEXTURE_2D, gcm_format, 0, 0, width, height, 1, mipmaps, remap_vector, false);

			copy_transfer_regions_impl(cmd, result->image(), sections_to_copy);
			return result;
		}

		void release_temporary_subresource(gl::texture_view* view) override
		{
			for (auto& e : m_temporary_surfaces)
			{
				if (e.get() == view->image())
				{
					e->release();
					return;
				}
			}
		}

		void update_image_contents(gl::command_context& cmd, gl::texture_view* dst, gl::texture* src, u16 width, u16 height) override
		{
			std::vector<copy_region_descriptor> region =
			{{
				.src = src,
				.xform = rsx::surface_transform::identity,
				.src_w = width,
				.src_h = height,
				.dst_w = width,
				.dst_h = height
			}};

			copy_transfer_regions_impl(cmd, dst->image(), region);
		}

		cached_texture_section* create_new_texture(gl::command_context& cmd, const utils::address_range32 &rsx_range, u16 width, u16 height, u16 depth, u16 mipmaps, u32 pitch,
			u32 gcm_format, rsx::texture_upload_context context, rsx::texture_dimension_extended type, bool swizzled, rsx::component_order swizzle_flags, rsx::flags32_t /*flags*/) override
		{
			const rsx::image_section_attributes_t search_desc = { .gcm_format = gcm_format, .width = width, .height = height, .depth = depth, .mipmaps = mipmaps };
			const bool allow_dirty = (context != rsx::texture_upload_context::framebuffer_storage);
			auto& cached = *find_cached_texture(rsx_range, search_desc, true, true, allow_dirty);
			ensure(!cached.is_locked());

			gl::viewable_image* image = nullptr;
			if (cached.exists())
			{
				// Try and reuse this image data. It is very likely to match our needs
				image = dynamic_cast<gl::viewable_image*>(cached.get_raw_texture());
				if (!image || cached.get_image_type() != type)
				{
					// Type mismatch, discard
					cached.destroy();
					image = nullptr;
				}
				else
				{
					ensure(cached.is_managed());

					cached.set_dimensions(width, height, depth, pitch);
					cached.set_format(texture::format::rgba, texture::type::ubyte, true);

					// Clear the image before use if it is not going to be uploaded wholly from CPU
					if (context != rsx::texture_upload_context::shader_read)
					{
						if (image->format_class() == RSX_FORMAT_CLASS_COLOR)
						{
							g_hw_blitter->fast_clear_image(cmd, image, color4f{});
						}
						else
						{
							g_hw_blitter->fast_clear_image(cmd, image, 1.f, 0);
						}
					}
				}
			}

			if (!image)
			{
				ensure(!cached.exists());
				image = gl::create_texture(gcm_format, width, height, depth, mipmaps, type);

				// Prepare section
				cached.reset(rsx_range);
				cached.set_image_type(type);
				cached.set_gcm_format(gcm_format);
				cached.create(width, height, depth, mipmaps, image, pitch, true);
			}

			cached.set_view_flags(swizzle_flags);
			cached.set_context(context);
			cached.set_swizzled(swizzled);
			cached.set_dirty(false);

			const auto swizzle = get_component_mapping(gcm_format, swizzle_flags);
			image->set_native_component_layout(swizzle);

			if (context != rsx::texture_upload_context::blit_engine_dst)
			{
				AUDIT(cached.get_memory_read_flags() != rsx::memory_read_flags::flush_always);
				read_only_range = cached.get_min_max(read_only_range, rsx::section_bounds::locked_range); // TODO ruipin: This was outside the if, but is inside the if in Vulkan. Ask kd-11
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
					cached.set_format(gl::texture::format::bgra, gl::texture::type::uint_8_8_8_8_rev, true);
					break;
				}
				case CELL_GCM_TEXTURE_R5G6B5:
				{
					cached.set_format(gl::texture::format::rgb, gl::texture::type::ushort_5_6_5, true);
					break;
				}
				case CELL_GCM_TEXTURE_DEPTH24_D8:
				{
					cached.set_format(gl::texture::format::depth_stencil, gl::texture::type::uint_24_8, false);
					break;
				}
				case CELL_GCM_TEXTURE_DEPTH16:
				{
					cached.set_format(gl::texture::format::depth, gl::texture::type::ushort, true);
					break;
				}
				default:
					fmt::throw_exception("Unexpected gcm format 0x%X", gcm_format);
				}

				//NOTE: Protection is handled by the caller
				cached.set_dimensions(width, height, depth, (rsx_range.length() / height));
				no_access_range = cached.get_min_max(no_access_range, rsx::section_bounds::locked_range);
			}

			update_cache_tag();
			return &cached;
		}

		cached_texture_section* create_nul_section(
			gl::command_context& /*cmd*/,
			const utils::address_range32& rsx_range,
			const rsx::image_section_attributes_t& attrs,
			const rsx::GCM_tile_reference& /*tile*/,
			bool /*memory_load*/) override
		{
			auto& cached = *find_cached_texture(rsx_range, { .gcm_format = RSX_GCM_FORMAT_IGNORED }, true, false, false);
			ensure(!cached.is_locked());

			// Prepare section
			cached.reset(rsx_range);
			cached.create_dma_only(attrs.width, attrs.height, attrs.pitch);
			cached.set_dirty(false);

			no_access_range = cached.get_min_max(no_access_range, rsx::section_bounds::locked_range);
			update_cache_tag();
			return &cached;
		}

		cached_texture_section* upload_image_from_cpu(gl::command_context& cmd, const utils::address_range32& rsx_range, u16 width, u16 height, u16 depth, u16 mipmaps, u32 pitch, u32 gcm_format,
			rsx::texture_upload_context context, const std::vector<rsx::subresource_layout>& subresource_layout, rsx::texture_dimension_extended type, bool input_swizzled) override
		{
			auto section = create_new_texture(cmd, rsx_range, width, height, depth, mipmaps, pitch, gcm_format, context, type, input_swizzled,
				rsx::component_order::default_, 0);

			gl::upload_texture(cmd, section->get_raw_texture(), gcm_format, input_swizzled, subresource_layout);

			section->last_write_tag = rsx::get_shared_tag();
			return section;
		}

		void set_component_order(cached_texture_section& section, u32 gcm_format, rsx::component_order flags) override
		{
			if (flags == section.get_view_flags())
				return;

			const auto swizzle = get_component_mapping(gcm_format, flags);
			auto image = static_cast<gl::viewable_image*>(section.get_raw_texture());

			ensure(image);
			image->set_native_component_layout(swizzle);

			section.set_view_flags(flags);
		}

		void insert_texture_barrier(gl::command_context&, gl::texture*, bool) override
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
				// TODO
				err_once("Format incompatibility detected, reporting failure to force data copy (GL_INTERNAL_FORMAT=0x%X, GCM_FORMAT=0x%X)", static_cast<u32>(ifmt), gcm_format);
				return false;
			case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
				return (ifmt == gl::texture::internal_format::rgba16f);
			case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
				return (ifmt == gl::texture::internal_format::rgba32f);
			case CELL_GCM_TEXTURE_X32_FLOAT:
				return (ifmt == gl::texture::internal_format::r32f);
			case CELL_GCM_TEXTURE_R5G6B5:
				return (ifmt == gl::texture::internal_format::rgb565);
			case CELL_GCM_TEXTURE_A8R8G8B8:
			case CELL_GCM_TEXTURE_D8R8G8B8:
				return (ifmt == gl::texture::internal_format::bgra8 ||
						ifmt == gl::texture::internal_format::depth24_stencil8 ||
						ifmt == gl::texture::internal_format::depth32f_stencil8);
			case CELL_GCM_TEXTURE_B8:
				return (ifmt == gl::texture::internal_format::r8);
			case CELL_GCM_TEXTURE_G8B8:
				return (ifmt == gl::texture::internal_format::rg8);
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
				return (ifmt == gl::texture::internal_format::depth24_stencil8 ||
						ifmt == gl::texture::internal_format::depth32f_stencil8);
			case CELL_GCM_TEXTURE_X16:
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
				return (ifmt == gl::texture::internal_format::depth16 ||
						ifmt == gl::texture::internal_format::depth32f);
			}
		}

		void prepare_for_dma_transfers(gl::command_context&) override
		{}

		void cleanup_after_dma_transfers(gl::command_context&) override
		{}

	public:

		using baseclass::texture_cache;

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

			auto &block = m_storage.block_for(rsx_address);

			if (block.get_locked_count() == 0)
				return false;

			for (auto& tex : block)
			{
				if (tex.is_dirty())
					continue;

				if (!tex.overlaps(rsx_address, rsx::section_bounds::full_range))
					continue;

				if ((rsx_address + rsx_size - tex.get_section_base()) <= tex.get_section_size())
					return tex.is_depth_texture();
			}

			return false;
		}

		void on_frame_end() override
		{
			trim_sections();

			if (m_storage.m_unreleased_texture_objects >= m_max_zombie_objects)
			{
				purge_unreleased_sections();
			}

			if (m_temporary_surfaces.size() > max_cached_image_pool_size)
			{
				m_temporary_surfaces.resize(max_cached_image_pool_size / 2);
			}

			baseclass::on_frame_end();
		}

		bool blit(gl::command_context& cmd, const rsx::blit_src_info& src, const rsx::blit_dst_info& dst, bool linear_interpolate, gl_render_targets& m_rtts)
		{
			auto result = upload_scaled_image(src, dst, linear_interpolate, cmd, m_rtts, m_hw_blitter);

			if (result.succeeded)
			{
				if (result.real_dst_size)
				{
					flush_if_cache_miss_likely(cmd, result.to_address_range());
				}

				return true;
			}

			return false;
		}
	};
}
