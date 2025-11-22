#include "stdafx.h"
#include "GLTexture.h"
#include "GLCompute.h"
#include "GLOverlays.h"
#include "GLGSRender.h"

#include "glutils/blitter.h"
#include "glutils/ring_buffer.h"

#include "../RSXThread.h"

#include "util/asm.hpp"

namespace gl
{
	namespace debug
	{
		extern void set_vis_texture(texture*);
	}

	scratch_ring_buffer g_typeless_transfer_buffer;
	legacy_ring_buffer g_upload_transfer_buffer;
	scratch_ring_buffer g_compute_decode_buffer;
	scratch_ring_buffer g_deswizzle_scratch_buffer;

	void destroy_global_texture_resources()
	{
		g_typeless_transfer_buffer.remove();
		g_upload_transfer_buffer.remove();
		g_compute_decode_buffer.remove();
		g_deswizzle_scratch_buffer.remove();
	}

	template <typename WordType, bool SwapBytes>
	void do_deswizzle_transformation(gl::command_context& cmd, u32 block_size, buffer* dst, u32 dst_offset, buffer* src, u32 src_offset, u32 data_length, u16 width, u16 height, u16 depth)
	{
		switch (block_size)
		{
		case 1:
			gl::get_compute_task<gl::cs_deswizzle_3d<u8, WordType, SwapBytes>>()->run(
				cmd, dst, dst_offset, src, src_offset,
				data_length, width, height, depth, 1);
			break;
		case 2:
			gl::get_compute_task<gl::cs_deswizzle_3d<u16, WordType, SwapBytes>>()->run(
				cmd, dst, dst_offset, src, src_offset,
				data_length, width, height, depth, 1);
			break;
		case 4:
			gl::get_compute_task<gl::cs_deswizzle_3d<u32, WordType, SwapBytes>>()->run(
				cmd, dst, dst_offset, src, src_offset,
				data_length, width, height, depth, 1);
			break;
		case 8:
			gl::get_compute_task<gl::cs_deswizzle_3d<u64, WordType, SwapBytes>>()->run(
				cmd, dst, dst_offset, src, src_offset,
				data_length, width, height, depth, 1);
			break;
		case 16:
			gl::get_compute_task<gl::cs_deswizzle_3d<u128, WordType, SwapBytes>>()->run(
				cmd, dst, dst_offset, src, src_offset,
				data_length, width, height, depth, 1);
			break;
		default:
			fmt::throw_exception("Unreachable");
		}
	}

	GLenum get_target(rsx::texture_dimension_extended type)
	{
		switch (type)
		{
		case rsx::texture_dimension_extended::texture_dimension_1d: return GL_TEXTURE_1D;
		case rsx::texture_dimension_extended::texture_dimension_2d: return GL_TEXTURE_2D;
		case rsx::texture_dimension_extended::texture_dimension_cubemap: return GL_TEXTURE_CUBE_MAP;
		case rsx::texture_dimension_extended::texture_dimension_3d: return GL_TEXTURE_3D;
		}
		fmt::throw_exception("Unknown texture target");
	}

	GLenum get_sized_internal_format(u32 texture_format)
	{
		const bool supports_dxt = get_driver_caps().EXT_texture_compression_s3tc_supported;
		switch (texture_format)
		{
		case CELL_GCM_TEXTURE_B8: return GL_R8;
		case CELL_GCM_TEXTURE_A1R5G5B5: return GL_BGR5_A1;
		case CELL_GCM_TEXTURE_A4R4G4B4: return GL_RGBA4;
		case CELL_GCM_TEXTURE_R5G6B5: return GL_RGB565;
		case CELL_GCM_TEXTURE_A8R8G8B8: return GL_BGRA8;
		case CELL_GCM_TEXTURE_G8B8: return GL_RG8;
		case CELL_GCM_TEXTURE_R6G5B5: return GL_RGB565;
		case CELL_GCM_TEXTURE_DEPTH24_D8: return GL_DEPTH24_STENCIL8;
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: return GL_DEPTH32F_STENCIL8;
		case CELL_GCM_TEXTURE_DEPTH16: return GL_DEPTH_COMPONENT16;
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT: return GL_DEPTH_COMPONENT32F;
		case CELL_GCM_TEXTURE_X16: return GL_R16;
		case CELL_GCM_TEXTURE_Y16_X16: return GL_RG16;
		case CELL_GCM_TEXTURE_R5G5B5A1: return GL_RGB5_A1;
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: return GL_RGBA16F;
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: return GL_RGBA32F;
		case CELL_GCM_TEXTURE_X32_FLOAT: return GL_R32F;
		case CELL_GCM_TEXTURE_D1R5G5B5: return GL_BGR5_A1;
		case CELL_GCM_TEXTURE_D8R8G8B8: return GL_BGRA8;
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT: return GL_RG16F;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1: return supports_dxt ? GL_COMPRESSED_RGBA_S3TC_DXT1_EXT : GL_BGRA8;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23: return supports_dxt ? GL_COMPRESSED_RGBA_S3TC_DXT3_EXT : GL_BGRA8;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return supports_dxt ? GL_COMPRESSED_RGBA_S3TC_DXT5_EXT : GL_BGRA8;
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8: return GL_RG8;
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8: return GL_RG8_SNORM;
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8: return GL_BGRA8;
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return GL_BGRA8;
		}
		fmt::throw_exception("Unknown texture format 0x%x", texture_format);
	}

	std::tuple<GLenum, GLenum> get_format_type(u32 texture_format)
	{
		const bool supports_dxt = get_driver_caps().EXT_texture_compression_s3tc_supported;
		switch (texture_format)
		{
		case CELL_GCM_TEXTURE_B8: return std::make_tuple(GL_RED, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_A1R5G5B5: return std::make_tuple(GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
		case CELL_GCM_TEXTURE_A4R4G4B4: return std::make_tuple(GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4);
		case CELL_GCM_TEXTURE_R5G6B5: return std::make_tuple(GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
		case CELL_GCM_TEXTURE_A8R8G8B8: return std::make_tuple(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV);
		case CELL_GCM_TEXTURE_G8B8: return std::make_tuple(GL_RG, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_R6G5B5: return std::make_tuple(GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
		case CELL_GCM_TEXTURE_DEPTH24_D8: return std::make_tuple(GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: return std::make_tuple(GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV);
		case CELL_GCM_TEXTURE_DEPTH16: return std::make_tuple(GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT);
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT: return std::make_tuple(GL_DEPTH_COMPONENT, GL_FLOAT);
		case CELL_GCM_TEXTURE_X16: return std::make_tuple(GL_RED, GL_UNSIGNED_SHORT);
		case CELL_GCM_TEXTURE_Y16_X16: return std::make_tuple(GL_RG, GL_UNSIGNED_SHORT);
		case CELL_GCM_TEXTURE_R5G5B5A1: return std::make_tuple(GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1);
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: return std::make_tuple(GL_RGBA, GL_HALF_FLOAT);
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: return std::make_tuple(GL_RGBA, GL_FLOAT);
		case CELL_GCM_TEXTURE_X32_FLOAT: return std::make_tuple(GL_RED, GL_FLOAT);
		case CELL_GCM_TEXTURE_D1R5G5B5: return std::make_tuple(GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
		case CELL_GCM_TEXTURE_D8R8G8B8: return std::make_tuple(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV);
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT: return std::make_tuple(GL_RG, GL_HALF_FLOAT);
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1: return std::make_tuple(supports_dxt ? GL_COMPRESSED_RGBA_S3TC_DXT1_EXT : GL_BGRA, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23: return std::make_tuple(supports_dxt ? GL_COMPRESSED_RGBA_S3TC_DXT3_EXT : GL_BGRA, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return std::make_tuple(supports_dxt ? GL_COMPRESSED_RGBA_S3TC_DXT5_EXT : GL_BGRA, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8: return std::make_tuple(GL_RG, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8: return std::make_tuple(GL_RG, GL_BYTE);
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8: return std::make_tuple(GL_BGRA, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return std::make_tuple(GL_BGRA, GL_UNSIGNED_BYTE);
		}
		fmt::throw_exception("Compressed or unknown texture format 0x%x", texture_format);
	}

	pixel_buffer_layout get_format_type(texture::internal_format format)
	{
		switch (format)
		{
		case texture::internal_format::compressed_rgba_s3tc_dxt1:
		case texture::internal_format::compressed_rgba_s3tc_dxt3:
		case texture::internal_format::compressed_rgba_s3tc_dxt5:
			return { GL_RGBA, GL_UNSIGNED_BYTE, 1, false };
		case texture::internal_format::r8:
			return { GL_RED, GL_UNSIGNED_BYTE, 1, false };
		case texture::internal_format::r16:
			return { GL_RED, GL_UNSIGNED_SHORT, 2, true };
		case texture::internal_format::r32f:
			return { GL_RED, GL_FLOAT, 4, true };
		case texture::internal_format::rg8:
			return { GL_RG, GL_UNSIGNED_SHORT, 2, true };
		case texture::internal_format::rg16:
			return { GL_RG, GL_UNSIGNED_SHORT, 2, true };
		case texture::internal_format::rg16f:
			return { GL_RG, GL_HALF_FLOAT, 2, true };
		case texture::internal_format::rgb565:
			return { GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 2, true };
		case texture::internal_format::rgb5a1:
			return { GL_RGB, GL_UNSIGNED_SHORT_5_5_5_1, 2, true };
		case texture::internal_format::bgr5a1:
			return { GL_RGB, GL_UNSIGNED_SHORT_1_5_5_5_REV, 2, true };
		case texture::internal_format::rgba4:
			return { GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4, 2, false };
		case texture::internal_format::rgba8:
			return { GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, 4, true };
		case texture::internal_format::bgra8:
			return { GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 4, true };
		case texture::internal_format::rgba16f:
			return { GL_RGBA, GL_HALF_FLOAT, 2, true };
		case texture::internal_format::rgba32f:
			return { GL_RGBA, GL_FLOAT, 4, true };
		case texture::internal_format::depth16:
			return { GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, 2, true };
		case texture::internal_format::depth32f:
			return { GL_DEPTH_COMPONENT, GL_FLOAT, 2, true };
		case texture::internal_format::depth24_stencil8:
		case texture::internal_format::depth32f_stencil8:
			return { GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 4, true };
		default:
			fmt::throw_exception("Unexpected internal format 0x%X", static_cast<u32>(format));
		}
	}

	pixel_buffer_layout get_format_type(const gl::texture* tex)
	{
		auto ret = get_format_type(tex->get_internal_format());
		if (tex->format_class() == RSX_FORMAT_CLASS_DEPTH24_FLOAT_X8_PACK32)
		{
			ret.type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
		}

		return ret;
	}

	std::array<GLenum, 4> get_swizzle_remap(u32 texture_format)
	{
		// NOTE: This must be in ARGB order in all forms below.
		switch (texture_format)
		{
		case CELL_GCM_TEXTURE_A1R5G5B5:
		case CELL_GCM_TEXTURE_R5G5B5A1:
		case CELL_GCM_TEXTURE_R6G5B5:
		case CELL_GCM_TEXTURE_R5G6B5:
		case CELL_GCM_TEXTURE_A4R4G4B4:
		case CELL_GCM_TEXTURE_A8R8G8B8:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			return{ GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE };

		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			return{ GL_RED, GL_RED, GL_RED, GL_RED };

		case CELL_GCM_TEXTURE_B8:
			return{ GL_ONE, GL_RED, GL_RED, GL_RED };

		case CELL_GCM_TEXTURE_X16:
			return{ GL_RED, GL_ONE, GL_RED, GL_ONE };

		case CELL_GCM_TEXTURE_X32_FLOAT:
			return{ GL_RED, GL_RED, GL_RED, GL_RED };

		case CELL_GCM_TEXTURE_G8B8:
			return{ GL_GREEN, GL_RED, GL_GREEN, GL_RED };

		case CELL_GCM_TEXTURE_Y16_X16:
			return{ GL_GREEN, GL_RED, GL_GREEN, GL_RED };

		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
			return{ GL_RED, GL_GREEN, GL_RED, GL_GREEN };

		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
			return{ GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE };

		case CELL_GCM_TEXTURE_D1R5G5B5:
		case CELL_GCM_TEXTURE_D8R8G8B8:
			return{ GL_ONE, GL_RED, GL_GREEN, GL_BLUE };

		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
			return{ GL_RED, GL_GREEN, GL_RED, GL_GREEN };
		}
		fmt::throw_exception("Unknown format 0x%x", texture_format);
	}

	cs_shuffle_base* get_trivial_transform_job(const pixel_buffer_layout& pack_info)
	{
		if (!pack_info.swap_bytes)
		{
			return nullptr;
		}

		switch (pack_info.size)
		{
		case 1:
			return nullptr;
		case 2:
			return get_compute_task<gl::cs_shuffle_16>();
		case 4:
			return get_compute_task<gl::cs_shuffle_32>();
		default:
			fmt::throw_exception("Unsupported format");
		}
	}

	void* copy_image_to_buffer(gl::command_context& cmd, const pixel_buffer_layout& pack_info, const gl::texture* src, gl::buffer* dst,
		u32 dst_offset, const int src_level, const coord3u& src_region,  image_memory_requirements* mem_info)
	{
		auto initialize_scratch_mem = [&]() -> bool // skip_transform
		{
			const u64 max_mem = (mem_info->memory_required) ? mem_info->memory_required : mem_info->image_size_in_bytes;
			if (!(*dst) || max_mem > static_cast<u64>(dst->size()))
			{
				if (*dst) dst->remove();
				dst->create(buffer::target::ssbo, max_mem, nullptr, buffer::memory_type::local, 0);
			}

			if (auto as_vi = dynamic_cast<const gl::viewable_image*>(src);
				src->get_target() == gl::texture::target::texture2D &&
				as_vi)
			{
				// RGBA8 <-> D24X8 bitcasts are some very common conversions due to some PS3 coding hacks & workarounds.
				switch (src->get_internal_format())
				{
				case gl::texture::internal_format::depth24_stencil8:
					gl::get_compute_task<gl::cs_d24x8_to_ssbo>()->run(cmd,
						const_cast<gl::viewable_image*>(as_vi), dst, dst_offset,
						{ {src_region.x, src_region.y}, {src_region.width, src_region.height} },
						pack_info);
					return true;
				case gl::texture::internal_format::rgba8:
				case gl::texture::internal_format::bgra8:
					gl::get_compute_task<gl::cs_rgba8_to_ssbo>()->run(cmd,
						const_cast<gl::viewable_image*>(as_vi), dst, dst_offset,
						{ {src_region.x, src_region.y}, {src_region.width, src_region.height} },
						pack_info);
					return true;
				default:
					break;
				}
			}

			dst->bind(buffer::target::pixel_pack);
			src->copy_to(reinterpret_cast<void*>(static_cast<uintptr_t>(dst_offset)), static_cast<texture::format>(pack_info.format), static_cast<texture::type>(pack_info.type), src_level, src_region, {});
			return false;
		};

		void* result = reinterpret_cast<void*>(static_cast<uintptr_t>(dst_offset));
		if (src->aspect() == image_aspect::color ||
			pack_info.type == GL_UNSIGNED_SHORT ||
			pack_info.type == GL_UNSIGNED_INT_24_8)
		{
			if (!initialize_scratch_mem())
			{
				if (auto job = get_trivial_transform_job(pack_info))
				{
					job->run(cmd, dst, static_cast<u32>(mem_info->image_size_in_bytes), dst_offset);
				}
			}
		}
		else if (pack_info.type == GL_FLOAT)
		{
			ensure(mem_info->image_size_in_bytes == (mem_info->image_size_in_texels * 4));
			mem_info->memory_required = (mem_info->image_size_in_texels * 6);
			ensure(!initialize_scratch_mem());

			if (pack_info.swap_bytes) [[ likely ]]
			{
				get_compute_task<cs_fconvert_task<f32, f16, false, true>>()->run(cmd, dst, dst_offset,
					static_cast<u32>(mem_info->image_size_in_bytes), static_cast<u32>(mem_info->image_size_in_bytes));
			}
			else
			{
				get_compute_task<cs_fconvert_task<f32, f16, false, false>>()->run(cmd, dst, dst_offset,
					static_cast<u32>(mem_info->image_size_in_bytes), static_cast<u32>(mem_info->image_size_in_bytes));
			}
			result = reinterpret_cast<void*>(mem_info->image_size_in_bytes + dst_offset);
		}
		else if (pack_info.type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV)
		{
			ensure(mem_info->image_size_in_bytes == (mem_info->image_size_in_texels * 8));
			mem_info->memory_required = (mem_info->image_size_in_texels * 12);
			ensure(!initialize_scratch_mem());

			if (pack_info.swap_bytes)
			{
				get_compute_task<cs_shuffle_d32fx8_to_x8d24f<true>>()->run(cmd, dst, dst_offset,
					static_cast<u32>(mem_info->image_size_in_bytes), static_cast<u32>(mem_info->image_size_in_texels));
			}
			else
			{
				get_compute_task<cs_shuffle_d32fx8_to_x8d24f<false>>()->run(cmd, dst, dst_offset,
					static_cast<u32>(mem_info->image_size_in_bytes), static_cast<u32>(mem_info->image_size_in_texels));
			}
			result = reinterpret_cast<void*>(mem_info->image_size_in_bytes + dst_offset);
		}
		else
		{
			fmt::throw_exception("Invalid depth/stencil type 0x%x", pack_info.type);
		}

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_PIXEL_BUFFER_BARRIER_BIT);
		return result;
	}

	void copy_buffer_to_image(gl::command_context& cmd, const pixel_buffer_layout& unpack_info, gl::buffer* src, gl::texture* dst,
		const void* src_offset, const int dst_level, const coord3u& dst_region, image_memory_requirements* mem_info)
	{
		buffer scratch_mem;
		buffer* transfer_buf = src;
		bool skip_barrier = false;
		u32 in_offset = static_cast<u32>(reinterpret_cast<u64>(src_offset));
		u32 out_offset = in_offset;
		const auto& caps = gl::get_driver_caps();

		auto initialize_scratch_mem = [&]()
		{
			if (in_offset >= mem_info->memory_required)
			{
				return;
			}

			const u64 max_mem = mem_info->memory_required + mem_info->image_size_in_bytes;
			if ((max_mem + in_offset) <= static_cast<u64>(src->size()))
			{
				out_offset = static_cast<u32>(in_offset + mem_info->image_size_in_bytes);
				return;
			}

			scratch_mem.create(buffer::target::pixel_pack, max_mem, nullptr, buffer::memory_type::local, 0);

			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			src->copy_to(&scratch_mem, in_offset, 0, mem_info->image_size_in_bytes);

			in_offset = 0;
			out_offset = static_cast<u32>(mem_info->image_size_in_bytes);
			transfer_buf = &scratch_mem;
		};

		if ((dst->aspect() & image_aspect::stencil) == 0 || caps.ARB_shader_stencil_export_supported)
		{
			// We do not need to use the driver's builtin transport mechanism
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			std::unique_ptr<gl::texture> scratch;
			std::unique_ptr<gl::texture_view> scratch_view;

			coordu image_region = { {dst_region.x, dst_region.y}, {dst_region.width, dst_region.height} };

			switch (dst->get_target())
			{
			case texture::target::texture3D:
			{
				// Upload to splatted image and do the final copy GPU-side
				image_region.height *= dst_region.depth;
				scratch = std::make_unique<gl::texture>(
					GL_TEXTURE_2D,
					image_region.x + image_region.width, image_region.y + image_region.height, 1, 1, 1,
					static_cast<GLenum>(dst->get_internal_format()), dst->format_class());

				scratch_view = std::make_unique<gl::nil_texture_view>(scratch.get());
				break;
			}
			case texture::target::textureCUBE:
			{
				const subresource_range range = { image_aspect::depth | image_aspect::color, static_cast<GLuint>(dst_level), 1, dst_region.z , 1 };
				scratch_view = std::make_unique<gl::texture_view>(dst, GL_TEXTURE_2D, range);
				break;
			}
			case texture::target::texture1D:
			{
				scratch = std::make_unique<gl::texture>(
					GL_TEXTURE_2D,
					image_region.x + image_region.width, 1, 1, 1, 1,
					static_cast<GLenum>(dst->get_internal_format()), dst->format_class());

				scratch_view = std::make_unique<gl::nil_texture_view>(scratch.get());
				break;
			}
			default:
			{
				ensure(dst->layers() == 1);

				if (dst->levels() > 1) [[ likely ]]
				{
					const subresource_range range = { image_aspect::depth | image_aspect::color, static_cast<GLuint>(dst_level), 1, 0 , 1 };
					scratch_view = std::make_unique<gl::texture_view>(dst, GL_TEXTURE_2D, range);
				}
				else
				{
					scratch_view = std::make_unique<gl::nil_texture_view>(dst);
				}

				break;
			}
			}

			// If possible, decode using a compute transform to potentially have asynchronous scheduling
			bool use_compute_transform = (
				dst->aspect() == gl::image_aspect::color &&  // Cannot use image_load_store with depth images
				caps.subvendor_ATI == false);                // The old AMD/ATI driver does not support image writeonly without format specifier

			if (use_compute_transform)
			{
				switch (dst->get_internal_format())
				{
				case texture::internal_format::bgr5a1:
				case texture::internal_format::rgb5a1:
				case texture::internal_format::rgb565:
				case texture::internal_format::rgba4:
					// Packed formats are a problem with image_load_store
					use_compute_transform = false;
					break;
				default:
					break;
				}
			}

			if (use_compute_transform)
			{
				gl::get_compute_task<gl::cs_ssbo_to_color_image>()->run(cmd, transfer_buf, scratch_view.get(), out_offset, image_region, unpack_info);
			}
			else
			{
				gl::get_overlay_pass<gl::rp_ssbo_to_generic_texture>()->run(cmd, transfer_buf, scratch_view.get(), out_offset, image_region, unpack_info);
			}

			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);

			switch (dst->get_target())
			{
			case texture::target::texture1D:
			{
				const position3u transfer_offset = { dst_region.position.x, 0, 0 };
				g_hw_blitter->copy_image(cmd, scratch.get(), dst, 0, dst_level, transfer_offset, transfer_offset, { dst_region.width, 1, 1 });
				break;
			}
			case texture::target::texture3D:
			{
				// Memcpy
				for (u32 layer = dst_region.z, i = 0; i < dst_region.depth; ++i, ++layer)
				{
					const position3u src_offset = { dst_region.position.x, dst_region.position.y + (i * dst_region.height), 0 };
					const position3u dst_offset = { dst_region.position.x, dst_region.position.y, layer };
					g_hw_blitter->copy_image(cmd, scratch.get(), dst, 0, dst_level, src_offset, dst_offset, { dst_region.width, dst_region.height, 1 });
				}
				break;
			}
			default: break;
			}
		}
		else
		{
			// Stencil format on NV. Use driver upload path
			if (unpack_info.type == GL_UNSIGNED_INT_24_8)
			{
				if (auto job = get_trivial_transform_job(unpack_info))
				{
					job->run(cmd, src, static_cast<u32>(mem_info->image_size_in_bytes), in_offset);
				}
				else
				{
					skip_barrier = true;
				}
			}
			else if (unpack_info.type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV)
			{
				mem_info->memory_required = (mem_info->image_size_in_texels * 8);
				initialize_scratch_mem();

				if (unpack_info.swap_bytes)
				{
					get_compute_task<cs_shuffle_x8d24f_to_d32fx8<true>>()->run(cmd, transfer_buf, in_offset, out_offset, static_cast<u32>(mem_info->image_size_in_texels));
				}
				else
				{
					get_compute_task<cs_shuffle_x8d24f_to_d32fx8<false>>()->run(cmd, transfer_buf, in_offset, out_offset, static_cast<u32>(mem_info->image_size_in_texels));
				}
			}
			else
			{
				fmt::throw_exception("Invalid depth/stencil type 0x%x", unpack_info.type);
			}

			if (!skip_barrier)
			{
				glMemoryBarrier(GL_PIXEL_BUFFER_BARRIER_BIT);
			}

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, GL_NONE);
			transfer_buf->bind(buffer::target::pixel_unpack);

			dst->copy_from(reinterpret_cast<void*>(u64(out_offset)), static_cast<texture::format>(unpack_info.format),
				static_cast<texture::type>(unpack_info.type), dst_level, dst_region, {});
		}
	}

	gl::viewable_image* create_texture(u32 gcm_format, u16 width, u16 height, u16 depth, u16 mipmaps,
			rsx::texture_dimension_extended type)
	{
		const GLenum target = get_target(type);
		const GLenum internal_format = get_sized_internal_format(gcm_format);
		const auto format_class = rsx::classify_format(gcm_format);

		return new gl::viewable_image(target, width, height, depth, mipmaps, 1, internal_format, format_class);
	}

	void fill_texture(gl::command_context& cmd, texture* dst, int format,
			const std::vector<rsx::subresource_layout> &input_layouts,
			bool is_swizzled, GLenum gl_format, GLenum gl_type, std::span<std::byte> staging_buffer)
	{
		const auto& driver_caps = gl::get_driver_caps();
		rsx::texture_uploader_capabilities caps
		{
			.supports_byteswap = true,
			.supports_vtc_decoding = false,
			.supports_hw_deswizzle = driver_caps.ARB_compute_shader_supported,
			.supports_zero_copy = false,
			.supports_dxt = driver_caps.EXT_texture_compression_s3tc_supported,
			.alignment = 4
		};

		pixel_unpack_settings unpack_settings;
		unpack_settings.row_length(0).alignment(4);

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, GL_NONE);

		if (rsx::is_compressed_host_format(caps, format)) [[likely]]
		{
			caps.supports_vtc_decoding = driver_caps.vendor_NVIDIA;
			unpack_settings.apply();

			const GLsizei format_block_size = (format == CELL_GCM_TEXTURE_COMPRESSED_DXT1) ? 8 : 16;

			for (const rsx::subresource_layout& layout : input_layouts)
			{
				rsx::io_buffer io_buf = staging_buffer;
				upload_texture_subresource(io_buf, layout, format, is_swizzled, caps);

				switch (dst->get_target())
				{
				case texture::target::texture1D:
				{
					const GLsizei size = layout.width_in_block * format_block_size;
					ensure(usz(size) <= staging_buffer.size());
					DSA_CALL(CompressedTextureSubImage1D, dst->id(), GL_TEXTURE_1D, layout.level, 0, layout.width_in_texel, gl_format, size, staging_buffer.data());
					break;
				}
				case texture::target::texture2D:
				{
					const GLsizei size = layout.width_in_block * layout.height_in_block * format_block_size;
					ensure(usz(size) <= staging_buffer.size());
					DSA_CALL(CompressedTextureSubImage2D, dst->id(), GL_TEXTURE_2D, layout.level, 0, 0, layout.width_in_texel, layout.height_in_texel, gl_format, size, staging_buffer.data());
					break;
				}
				case texture::target::textureCUBE:
				{
					const GLsizei size = layout.width_in_block * layout.height_in_block * format_block_size;
					ensure(usz(size) <= staging_buffer.size());
					if (gl::get_driver_caps().ARB_direct_state_access_supported)
					{
						glCompressedTextureSubImage3D(dst->id(), layout.level, 0, 0, layout.layer, layout.width_in_texel, layout.height_in_texel, 1, gl_format, size, staging_buffer.data());
					}
					else
					{
						glCompressedTextureSubImage2DEXT(dst->id(), GL_TEXTURE_CUBE_MAP_POSITIVE_X + layout.layer, layout.level, 0, 0, layout.width_in_texel, layout.height_in_texel, gl_format, size, staging_buffer.data());
					}
					break;
				}
				case texture::target::texture3D:
				{
					const GLsizei size = layout.width_in_block * layout.height_in_block * layout.depth * format_block_size;
					ensure(usz(size) <= staging_buffer.size());
					DSA_CALL(CompressedTextureSubImage3D, dst->id(), GL_TEXTURE_3D, layout.level, 0, 0, 0, layout.width_in_texel, layout.height_in_texel, layout.depth, gl_format, size, staging_buffer.data());
					break;
				}
				default:
				{
					fmt::throw_exception("Unreachable");
				}
				}
			}
		}
		else
		{
			std::pair<void*, u32> upload_scratch_mem = {}, compute_scratch_mem = {};
			image_memory_requirements mem_info;
			pixel_buffer_layout mem_layout;

			std::span<std::byte> dst_buffer = staging_buffer;
			void* out_pointer = staging_buffer.data();
			u8 block_size_in_bytes = rsx::get_format_block_size_in_bytes(format);
			u64 image_linear_size = staging_buffer.size();

			const auto min_required_buffer_size = std::max<u64>(utils::align(image_linear_size * 4, 0x100000), 16 * 0x100000);

			if (driver_caps.ARB_compute_shader_supported)
			{
				if (g_upload_transfer_buffer.size() < static_cast<GLsizeiptr>(min_required_buffer_size))
				{
					g_upload_transfer_buffer.remove();
					g_upload_transfer_buffer.create(gl::buffer::target::pixel_unpack, min_required_buffer_size);
				}

				if (g_compute_decode_buffer.size() < min_required_buffer_size)
				{
					g_compute_decode_buffer.remove();
					g_compute_decode_buffer.create(gl::buffer::target::ssbo, min_required_buffer_size);
				}

				out_pointer = nullptr;
			}

			for (const rsx::subresource_layout& layout : input_layouts)
			{
				if (driver_caps.ARB_compute_shader_supported)
				{
					u64 row_pitch = rsx::align2<u64, u64>(layout.width_in_block * block_size_in_bytes, caps.alignment);

					// We're in the "else" branch, so "is_compressed_host_format()" is always false.
					// Handle emulated compressed formats with host unpack (R8G8 compressed)
					row_pitch = std::max<u64>(row_pitch, dst->pitch());

					// FIXME: Double-check this logic; it seems like we should always use texels both here and for row_pitch.
					image_linear_size = row_pitch * layout.height_in_texel * layout.depth;

					compute_scratch_mem = { nullptr, g_compute_decode_buffer.alloc(static_cast<u32>(image_linear_size), 256) };
					compute_scratch_mem.first = reinterpret_cast<void*>(static_cast<uintptr_t>(compute_scratch_mem.second));

					g_upload_transfer_buffer.reserve_storage_on_heap(static_cast<u32>(image_linear_size));
					upload_scratch_mem = g_upload_transfer_buffer.alloc_from_heap(static_cast<u32>(image_linear_size), 256);
					dst_buffer = { reinterpret_cast<std::byte*>(upload_scratch_mem.first), image_linear_size };
				}

				rsx::io_buffer io_buf = dst_buffer;
				caps.supports_hw_deswizzle = (is_swizzled && driver_caps.ARB_compute_shader_supported && image_linear_size > 1024);
				auto op = upload_texture_subresource(io_buf, layout, format, is_swizzled, caps);

				// Define upload region
				coord3u region;
				region.x = 0;
				region.y = 0;
				region.z = layout.layer;
				region.width = layout.width_in_texel;
				region.height = layout.height_in_texel;
				region.depth = layout.depth;

				if (driver_caps.ARB_compute_shader_supported)
				{
					// 0. Preconf
					mem_layout.alignment = static_cast<u8>(caps.alignment);
					mem_layout.swap_bytes = op.require_swap;
					mem_layout.format = gl_format;
					mem_layout.type = gl_type;
					mem_layout.size = block_size_in_bytes;

					// 2. Upload memory to GPU
					if (!op.require_deswizzle)
					{
						g_upload_transfer_buffer.unmap();
						g_upload_transfer_buffer.copy_to(&g_compute_decode_buffer.get(), upload_scratch_mem.second, compute_scratch_mem.second, image_linear_size);
					}
					else
					{
						// 2.1 Copy data to deswizzle buf
						if (g_deswizzle_scratch_buffer.size() < min_required_buffer_size)
						{
							g_deswizzle_scratch_buffer.remove();
							g_deswizzle_scratch_buffer.create(gl::buffer::target::ssbo, min_required_buffer_size);
						}

						u32 deswizzle_data_offset = g_deswizzle_scratch_buffer.alloc(static_cast<u32>(image_linear_size), 256);
						g_upload_transfer_buffer.unmap();
						g_upload_transfer_buffer.copy_to(&g_deswizzle_scratch_buffer.get(), upload_scratch_mem.second, deswizzle_data_offset, static_cast<u32>(image_linear_size));

						// 2.2 Apply compute transform to deswizzle input and dump it in compute_scratch_mem
						const auto block_size = op.element_size * op.block_length;

						if (op.require_swap)
						{
							mem_layout.swap_bytes = false;

							switch (op.element_size)
							{
							case 1:
								do_deswizzle_transformation<u8, true>(cmd, block_size,
									&g_compute_decode_buffer.get(), compute_scratch_mem.second, &g_deswizzle_scratch_buffer.get(), deswizzle_data_offset,
									static_cast<u32>(image_linear_size), layout.width_in_texel, layout.height_in_texel, layout.depth);
								break;
							case 2:
								do_deswizzle_transformation<u16, true>(cmd, block_size,
									&g_compute_decode_buffer.get(), compute_scratch_mem.second, &g_deswizzle_scratch_buffer.get(), deswizzle_data_offset,
									static_cast<u32>(image_linear_size), layout.width_in_texel, layout.height_in_texel, layout.depth);
								break;
							case 4:
								do_deswizzle_transformation<u32, true>(cmd, block_size,
									&g_compute_decode_buffer.get(), compute_scratch_mem.second, &g_deswizzle_scratch_buffer.get(), deswizzle_data_offset,
									static_cast<u32>(image_linear_size), layout.width_in_texel, layout.height_in_texel, layout.depth);
								break;
							default:
								fmt::throw_exception("Unimplemented element size deswizzle");
							}
						}
						else
						{
							switch (op.element_size)
							{
							case 1:
								do_deswizzle_transformation<u8, false>(cmd, block_size,
									&g_compute_decode_buffer.get(), compute_scratch_mem.second, &g_deswizzle_scratch_buffer.get(), deswizzle_data_offset,
									static_cast<u32>(image_linear_size), layout.width_in_texel, layout.height_in_texel, layout.depth);
								break;
							case 2:
								do_deswizzle_transformation<u16, false>(cmd, block_size,
									&g_compute_decode_buffer.get(), compute_scratch_mem.second, &g_deswizzle_scratch_buffer.get(), deswizzle_data_offset,
									static_cast<u32>(image_linear_size), layout.width_in_texel, layout.height_in_texel, layout.depth);
								break;
							case 4:
								do_deswizzle_transformation<u32, false>(cmd, block_size,
									&g_compute_decode_buffer.get(), compute_scratch_mem.second, &g_deswizzle_scratch_buffer.get(), deswizzle_data_offset,
									static_cast<u32>(image_linear_size), layout.width_in_texel, layout.height_in_texel, layout.depth);
								break;
							default:
								fmt::throw_exception("Unimplemented element size deswizzle");
							}
						}

						// Barrier
						g_deswizzle_scratch_buffer.push_barrier(deswizzle_data_offset, static_cast<u32>(image_linear_size));
					}

					// 3. Update configuration
					mem_info.image_size_in_texels = image_linear_size / block_size_in_bytes;
					mem_info.image_size_in_bytes = image_linear_size;
					mem_info.memory_required = 0;

					// 4. Dispatch compute routines
					copy_buffer_to_image(cmd, mem_layout, &g_compute_decode_buffer.get(), dst, compute_scratch_mem.first, layout.level, region, &mem_info);

					// Barrier
					g_compute_decode_buffer.push_barrier(compute_scratch_mem.second, static_cast<u32>(image_linear_size));
				}
				else
				{
					unpack_settings.swap_bytes(op.require_swap);
					dst->copy_from(out_pointer, static_cast<texture::format>(gl_format), static_cast<texture::type>(gl_type), layout.level, region, unpack_settings);
				}
			}
		}
	}

	std::array<GLenum, 4> apply_swizzle_remap(const std::array<GLenum, 4>& swizzle_remap, const rsx::texture_channel_remap_t& decoded_remap)
	{
		return decoded_remap.remap<GLenum>(swizzle_remap, GL_ZERO, GL_ONE);
	}

	void upload_texture(gl::command_context& cmd, texture* dst, u32 gcm_format, bool is_swizzled, const std::vector<rsx::subresource_layout>& subresources_layout)
	{
		// Calculate staging buffer size
		rsx::simple_array<std::byte, sizeof(u128)> data_upload_buf;

		rsx::texture_uploader_capabilities caps { .supports_dxt = gl::get_driver_caps().EXT_texture_compression_s3tc_supported };
		if (rsx::is_compressed_host_format(caps, gcm_format))
		{
			const auto& desc = subresources_layout[0];
			const u32 texture_data_sz = desc.width_in_block * desc.height_in_block * desc.depth * rsx::get_format_block_size_in_bytes(gcm_format);
			data_upload_buf.resize(texture_data_sz);
		}
		else
		{
			const auto aligned_pitch = utils::align<u32>(dst->pitch(), 4);
			const u32 texture_data_sz = dst->depth() * dst->height() * aligned_pitch;
			data_upload_buf.resize(texture_data_sz);
		}

		// TODO: GL drivers support byteswapping and this should be used instead of doing so manually
		const auto format_type = get_format_type(gcm_format);
		const GLenum gl_format = std::get<0>(format_type);
		const GLenum gl_type = std::get<1>(format_type);
		fill_texture(cmd, dst, gcm_format, subresources_layout, is_swizzled, gl_format, gl_type, data_upload_buf);

		// Notify the renderer of the upload
		auto renderer = static_cast<GLGSRender*>(rsx::get_current_renderer());
		renderer->on_guest_texture_read();
	}

	u32 get_format_texel_width(GLenum format)
	{
		switch (format)
		{
		case GL_R8:
			return 1;
		case GL_R32F:
		case GL_RG16:
		case GL_RG16F:
		case GL_RGBA8:
		case GL_BGRA8:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			return 4;
		case GL_R16:
		case GL_RG8:
		case GL_RGB565:
			return 2;
		case GL_RGBA16F:
			return 8;
		case GL_RGBA32F:
			return 16;
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT32F:
			return 2;
		case GL_DEPTH24_STENCIL8:
		case GL_DEPTH32F_STENCIL8:
			return 4;
		default:
			fmt::throw_exception("Unexpected internal format 0x%X", static_cast<u32>(format));
		}
	}

	std::pair<bool, u32> get_format_convert_flags(GLenum format)
	{
		switch (format)
		{
			// 8-bit
		case GL_R8:
			return { false, 1 };
		case GL_RGBA8:
		case GL_BGRA8:
			return { true, 4 };
			// 16-bit
		case GL_RG8:
		case GL_RG16:
		case GL_RG16F:
		case GL_R16:
		case GL_RGB565:
			return { true, 2 };
			// 32-bit
		case GL_R32F:
		case GL_RGBA32F:
			return { true, 4 };
			// DXT
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			return { false, 1 };
			// Depth
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT32F:
			return { true, 2 };
		case GL_DEPTH24_STENCIL8:
		case GL_DEPTH32F_STENCIL8:
			return { true, 4 };
		default:
			break;
		}

		fmt::throw_exception("Unexpected internal format 0x%X", static_cast<u32>(format));
	}

	bool formats_are_bitcast_compatible(GLenum format1, GLenum format2)
	{
		if (format1 == format2) [[likely]]
		{
			return true;
		}

		// Formats are compatible if the following conditions are met:
		// 1. Texel sizes must match
		// 2. Both formats require no transforms (basic memcpy) or...
		// 3. Both formats have the same transform (e.g RG16_UNORM to RG16_SFLOAT, both are down and uploaded with a 2-byte byteswap)

		if (format1 == GL_BGRA8 || format2 == GL_BGRA8)
		{
			return false;
		}

		if (get_format_texel_width(format1) != get_format_texel_width(format2))
		{
			return false;
		}

		const auto transform_a = get_format_convert_flags(format1);
		const auto transform_b = get_format_convert_flags(format2);

		if (transform_a.first == transform_b.first)
		{
			return !transform_a.first || (transform_a.second == transform_b.second);
		}

		return false;
	}

	bool formats_are_bitcast_compatible(const texture* texture1, const texture* texture2)
	{
		if (const u32 transfer_class = texture1->format_class() | texture2->format_class();
			transfer_class > RSX_FORMAT_CLASS_COLOR)
		{
			// If any one of the two images is a depth format, the other must match exactly or bust
			return (texture1->format_class() == texture2->format_class());
		}

		return formats_are_bitcast_compatible(static_cast<GLenum>(texture1->get_internal_format()), static_cast<GLenum>(texture2->get_internal_format()));
	}

	void copy_typeless(gl::command_context& cmd, texture * dst, const texture * src, const coord3u& dst_region, const coord3u& src_region)
	{
		const auto src_bpp = src->pitch() / src->width();
		const auto dst_bpp = dst->pitch() / dst->width();
		image_memory_requirements src_mem = { src_region.width * src_region.height, src_region.width * src_bpp * src_region.height, 0ull };
		image_memory_requirements dst_mem = { dst_region.width * dst_region.height, dst_region.width * dst_bpp * dst_region.height, 0ull };

		const auto& caps = gl::get_driver_caps();
		auto pack_info = get_format_type(src);
		auto unpack_info = get_format_type(dst);

		// D32FS8 can be read back as D24S8 or D32S8X24. In case of the latter, double memory requirements
		if (pack_info.type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV)
		{
			src_mem.image_size_in_bytes *= 2;
		}

		if (unpack_info.type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV)
		{
			dst_mem.image_size_in_bytes *= 2;
		}

		if (caps.ARB_compute_shader_supported) [[likely]]
		{
			bool skip_transform = false;
			if ((src->aspect() | dst->aspect()) == gl::image_aspect::color)
			{
				skip_transform = (pack_info.format == unpack_info.format &&
					pack_info.type == unpack_info.type &&
					pack_info.swap_bytes == unpack_info.swap_bytes &&
					pack_info.size == unpack_info.size);
			}

			if (skip_transform) [[likely]]
			{
				// Disable byteswap to make the transport operation passthrough
				pack_info.swap_bytes = false;
				unpack_info.swap_bytes = false;
			}

			u32 scratch_offset = 0;
			const u64 min_storage_requirement = src_mem.image_size_in_bytes + dst_mem.image_size_in_bytes;
			const u64 min_required_buffer_size = utils::align(min_storage_requirement, 256);

			if (g_typeless_transfer_buffer.size() >= min_required_buffer_size) [[ likely ]]
			{
				scratch_offset = g_typeless_transfer_buffer.alloc(static_cast<u32>(min_storage_requirement), 256);
			}
			else
			{
				const auto new_size = std::max(min_required_buffer_size, g_typeless_transfer_buffer.size() + (64 * 0x100000));
				g_typeless_transfer_buffer.create(gl::buffer::target::ssbo, new_size);
			}

			void* data_ptr = copy_image_to_buffer(cmd, pack_info, src, &g_typeless_transfer_buffer.get(), scratch_offset, 0, src_region, &src_mem);
			copy_buffer_to_image(cmd, unpack_info, &g_typeless_transfer_buffer.get(), dst, data_ptr, 0, dst_region, &dst_mem);

			// Not truly range-accurate, but should cover most of what we care about
			g_typeless_transfer_buffer.push_barrier(scratch_offset, static_cast<u32>(min_storage_requirement));

			// Cleanup
			// NOTE: glBindBufferRange also binds the buffer to the old-school target.
			// Unbind it to avoid glitching later
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, GL_NONE);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, GL_NONE);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
		}
		else
		{
			const u64 max_mem = std::max(src_mem.image_size_in_bytes, dst_mem.image_size_in_bytes);
			if (max_mem > static_cast<u64>(g_typeless_transfer_buffer.size()))
			{
				g_typeless_transfer_buffer.remove();
				g_typeless_transfer_buffer.create(buffer::target::pixel_pack, max_mem);
			}

			// Simplify pack/unpack information to something OpenGL can natively digest
			auto remove_depth_transformation = [](const texture* tex, pixel_buffer_layout& pack_info)
			{
				if (tex->aspect() & image_aspect::depth)
				{
					switch (pack_info.type)
					{
					case GL_UNSIGNED_INT_24_8:
						pack_info.swap_bytes = false;
						break;
					case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
						pack_info.type = GL_UNSIGNED_INT_24_8;
						pack_info.swap_bytes = false;
						break;
					case GL_FLOAT:
						pack_info.type = GL_HALF_FLOAT;
						break;
					default: break;
					}
				}
			};

			remove_depth_transformation(src, pack_info);
			remove_depth_transformation(dst, unpack_info);

			// Attempt to compensate for the lack of compute shader modifiers
			// If crossing the aspect boundary between color and depth
			// and one image is depth, invert byteswap for the other one to compensate
			const auto cross_aspect_test = (image_aspect::color | image_aspect::depth);
			const auto test = (src->aspect() | dst->aspect()) & cross_aspect_test;
			if (test == cross_aspect_test)
			{
				if (src->aspect() & image_aspect::depth)
				{
					// Source is depth, modify unpack rule
					if (pack_info.size == 4 && unpack_info.size == 4)
					{
						unpack_info.swap_bytes = !unpack_info.swap_bytes;
					}
				}
				else
				{
					// Dest is depth, modify pack rule
					if (pack_info.size == 4 && unpack_info.size == 4)
					{
						pack_info.swap_bytes = !pack_info.swap_bytes;
					}
				}
			}

			// Start pack operation
			pixel_pack_settings pack_settings{};
			pack_settings.swap_bytes(pack_info.swap_bytes);

			g_typeless_transfer_buffer.get().bind(buffer::target::pixel_pack);
			src->copy_to(nullptr, static_cast<texture::format>(pack_info.format), static_cast<texture::type>(pack_info.type), 0, src_region, pack_settings);

			glBindBuffer(GL_PIXEL_PACK_BUFFER, GL_NONE);

			// Start unpack operation
			pixel_unpack_settings unpack_settings{};
			unpack_settings.swap_bytes(unpack_info.swap_bytes);

			g_typeless_transfer_buffer.get().bind(buffer::target::pixel_unpack);
			dst->copy_from(nullptr, static_cast<texture::format>(unpack_info.format), static_cast<texture::type>(unpack_info.type), 0, dst_region, unpack_settings);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
		}
	}

	void copy_typeless(gl::command_context& cmd, texture* dst, const texture* src)
	{
		const coord3u src_area = { {}, src->size3D() };
		const coord3u dst_area = { {}, dst->size3D() };
		copy_typeless(cmd, dst, src, dst_area, src_area);
	}

	void clear_attachments(gl::command_context& cmd, const clear_cmd_info& info)
	{
		// Compile the clear command at the end. Other intervening operations will
		GLbitfield clear_mask = 0;
		if (info.aspect_mask & gl::image_aspect::color)
		{
			for (u32 buffer_id = 0; buffer_id < info.clear_color.attachment_count; ++buffer_id)
			{
				cmd->color_maski(buffer_id, info.clear_color.mask);
			}

			cmd->clear_color(info.clear_color.r, info.clear_color.g, info.clear_color.b, info.clear_color.a);
			clear_mask |= GL_COLOR_BUFFER_BIT;
		}
		if (info.aspect_mask & gl::image_aspect::depth)
		{
			cmd->depth_mask(GL_TRUE);
			cmd->clear_depth(info.clear_depth.value);
			clear_mask |= GL_DEPTH_BUFFER_BIT;
		}
		if (info.aspect_mask & gl::image_aspect::stencil)
		{
			cmd->stencil_mask(info.clear_stencil.mask);
			cmd->clear_stencil(info.clear_stencil.value);
			clear_mask |= GL_STENCIL_BUFFER_BIT;
		}

		glClear(clear_mask);
	}
}
