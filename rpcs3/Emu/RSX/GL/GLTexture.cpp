#include "stdafx.h"
#include "GLTexture.h"
#include "GLCompute.h"
#include "GLRenderTargets.h"
#include "../GCM.h"
#include "../RSXThread.h"
#include "../RSXTexture.h"

#include "util/asm.hpp"

#include <span>

namespace gl
{
	buffer g_typeless_transfer_buffer;

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
		switch (texture_format)
		{
		case CELL_GCM_TEXTURE_B8: return GL_R8;
		case CELL_GCM_TEXTURE_A1R5G5B5: return GL_RGB5_A1;
		case CELL_GCM_TEXTURE_A4R4G4B4: return GL_RGBA4;
		case CELL_GCM_TEXTURE_R5G6B5: return GL_RGB565;
		case CELL_GCM_TEXTURE_A8R8G8B8: return GL_RGBA8;
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
		case CELL_GCM_TEXTURE_D1R5G5B5: return GL_RGB5_A1;
		case CELL_GCM_TEXTURE_D8R8G8B8: return GL_RGBA8;
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT: return GL_RG16F;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1: return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8: return GL_RG8;
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8: return GL_RG8;
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8: return GL_RGBA8;
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return GL_RGBA8;
		}
		fmt::throw_exception("Unknown texture format 0x%x", texture_format);
	}

	std::tuple<GLenum, GLenum> get_format_type(u32 texture_format)
	{
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
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1: return std::make_tuple(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23: return std::make_tuple(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return std::make_tuple(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_UNSIGNED_BYTE);
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
		case texture::internal_format::rgba4:
			return { GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4, 2, false };
		case texture::internal_format::rgba8:
			return { GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, 4, false };
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
		const auto ifmt = tex->get_internal_format();
		if (ifmt == gl::texture::internal_format::rgba8)
		{
			// Multiple RTT layouts can map to this format. Override ABGR formats
			if (auto rtt = dynamic_cast<const gl::render_target*>(tex))
			{
				switch (rtt->format_info.gcm_color_format)
				{
				case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
				case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
				case rsx::surface_color_format::a8b8g8r8:
					return { GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, 4, false };
				default:
					break;
				}
			}
		}

		auto ret = get_format_type(ifmt);
		if (tex->format_class() == RSX_FORMAT_CLASS_DEPTH24_FLOAT_X8_PACK32)
		{
			ret.type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
		}

		return ret;
	}

	GLenum get_srgb_format(GLenum in_format)
	{
		switch (in_format)
		{
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
		case GL_RGBA8:
			return GL_SRGB8_ALPHA8;
		default:
			//rsx_log.error("No gamma conversion for format 0x%X", in_format);
			return in_format;
		}
	}

	GLenum wrap_mode(rsx::texture_wrap_mode wrap)
	{
		switch (wrap)
		{
		case rsx::texture_wrap_mode::wrap: return GL_REPEAT;
		case rsx::texture_wrap_mode::mirror: return GL_MIRRORED_REPEAT;
		case rsx::texture_wrap_mode::clamp_to_edge: return GL_CLAMP_TO_EDGE;
		case rsx::texture_wrap_mode::border: return GL_CLAMP_TO_BORDER;
		case rsx::texture_wrap_mode::clamp: return GL_CLAMP_TO_EDGE;
		case rsx::texture_wrap_mode::mirror_once_clamp_to_edge: return GL_MIRROR_CLAMP_TO_EDGE_EXT;
		case rsx::texture_wrap_mode::mirror_once_border: return GL_MIRROR_CLAMP_TO_BORDER_EXT;
		case rsx::texture_wrap_mode::mirror_once_clamp: return GL_MIRROR_CLAMP_EXT;
		}

		rsx_log.error("Texture wrap error: bad wrap (%d)", static_cast<u32>(wrap));
		return GL_REPEAT;
	}

	float max_aniso(rsx::texture_max_anisotropy aniso)
	{
		switch (aniso)
		{
		case rsx::texture_max_anisotropy::x1: return 1.0f;
		case rsx::texture_max_anisotropy::x2: return 2.0f;
		case rsx::texture_max_anisotropy::x4: return 4.0f;
		case rsx::texture_max_anisotropy::x6: return 6.0f;
		case rsx::texture_max_anisotropy::x8: return 8.0f;
		case rsx::texture_max_anisotropy::x10: return 10.0f;
		case rsx::texture_max_anisotropy::x12: return 12.0f;
		case rsx::texture_max_anisotropy::x16: return 16.0f;
		}

		rsx_log.error("Texture anisotropy error: bad max aniso (%d)", static_cast<u32>(aniso));
		return 1.0f;
	}

	int tex_min_filter(rsx::texture_minify_filter min_filter)
	{
		switch (min_filter)
		{
		case rsx::texture_minify_filter::nearest: return GL_NEAREST;
		case rsx::texture_minify_filter::linear: return GL_LINEAR;
		case rsx::texture_minify_filter::nearest_nearest: return GL_NEAREST_MIPMAP_NEAREST;
		case rsx::texture_minify_filter::linear_nearest: return GL_LINEAR_MIPMAP_NEAREST;
		case rsx::texture_minify_filter::nearest_linear: return GL_NEAREST_MIPMAP_LINEAR;
		case rsx::texture_minify_filter::linear_linear: return GL_LINEAR_MIPMAP_LINEAR;
		case rsx::texture_minify_filter::convolution_min: return GL_LINEAR_MIPMAP_LINEAR;
		}
		fmt::throw_exception("Unknown min filter");
	}

	int tex_mag_filter(rsx::texture_magnify_filter mag_filter)
	{
		switch (mag_filter)
		{
		case rsx::texture_magnify_filter::nearest: return GL_NEAREST;
		case rsx::texture_magnify_filter::linear: return GL_LINEAR;
		case rsx::texture_magnify_filter::convolution_mag: return GL_LINEAR;
		}
		fmt::throw_exception("Unknown mag filter");
	}

	// Apply sampler state settings
	void sampler_state::apply(const rsx::fragment_texture& tex, const rsx::sampled_image_descriptor_base* sampled_image)
	{
		set_parameteri(GL_TEXTURE_WRAP_S, wrap_mode(tex.wrap_s()));
		set_parameteri(GL_TEXTURE_WRAP_T, wrap_mode(tex.wrap_t()));
		set_parameteri(GL_TEXTURE_WRAP_R, wrap_mode(tex.wrap_r()));

		if (const auto color = tex.border_color();
			get_parameteri(GL_TEXTURE_BORDER_COLOR) != color)
		{
			m_propertiesi[GL_TEXTURE_BORDER_COLOR] = color;

			const color4f border_color = rsx::decode_border_color(color);
			glSamplerParameterfv(samplerHandle, GL_TEXTURE_BORDER_COLOR, border_color.rgba);
		}

		if (sampled_image->upload_context != rsx::texture_upload_context::shader_read ||
			tex.get_exact_mipmap_count() == 1)
		{
			GLint min_filter = tex_min_filter(tex.min_filter());

			if (min_filter != GL_LINEAR && min_filter != GL_NEAREST)
			{
				switch (min_filter)
				{
				case GL_NEAREST_MIPMAP_NEAREST:
				case GL_NEAREST_MIPMAP_LINEAR:
					min_filter = GL_NEAREST; break;
				case GL_LINEAR_MIPMAP_NEAREST:
				case GL_LINEAR_MIPMAP_LINEAR:
					min_filter = GL_LINEAR; break;
				default:
					rsx_log.error("No mipmap fallback defined for rsx_min_filter = 0x%X", static_cast<u32>(tex.min_filter()));
					min_filter = GL_NEAREST;
				}
			}

			set_parameteri(GL_TEXTURE_MIN_FILTER, min_filter);
			set_parameterf(GL_TEXTURE_LOD_BIAS, 0.f);
			set_parameterf(GL_TEXTURE_MIN_LOD, -1000.f);
			set_parameterf(GL_TEXTURE_MAX_LOD, 1000.f);
		}
		else
		{
			set_parameteri(GL_TEXTURE_MIN_FILTER, tex_min_filter(tex.min_filter()));
			set_parameterf(GL_TEXTURE_LOD_BIAS, tex.bias());
			set_parameterf(GL_TEXTURE_MIN_LOD, tex.min_lod());
			set_parameterf(GL_TEXTURE_MAX_LOD, tex.max_lod());
		}

		const f32 af_level = max_aniso(tex.max_aniso());
		set_parameterf(GL_TEXTURE_MAX_ANISOTROPY_EXT, af_level);
		set_parameteri(GL_TEXTURE_MAG_FILTER, tex_mag_filter(tex.mag_filter()));

		const u32 texture_format = tex.format() & ~(CELL_GCM_TEXTURE_UN | CELL_GCM_TEXTURE_LN);
		if (texture_format == CELL_GCM_TEXTURE_DEPTH16 || texture_format == CELL_GCM_TEXTURE_DEPTH24_D8 ||
			texture_format == CELL_GCM_TEXTURE_DEPTH16_FLOAT || texture_format == CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT)
		{
			//NOTE: The stored texture function is reversed wrt the textureProj compare function
			GLenum compare_mode = static_cast<GLenum>(tex.zfunc()) | GL_NEVER;

			switch (compare_mode)
			{
			case GL_GREATER: compare_mode = GL_LESS; break;
			case GL_GEQUAL: compare_mode = GL_LEQUAL; break;
			case GL_LESS: compare_mode = GL_GREATER; break;
			case GL_LEQUAL: compare_mode = GL_GEQUAL; break;
			}

			set_parameteri(GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			set_parameteri(GL_TEXTURE_COMPARE_FUNC, compare_mode);
		}
		else
			set_parameteri(GL_TEXTURE_COMPARE_MODE, GL_NONE);
	}

	void sampler_state::apply(const rsx::vertex_texture& tex, const rsx::sampled_image_descriptor_base* /*sampled_image*/)
	{
		if (const auto color = tex.border_color();
			get_parameteri(GL_TEXTURE_BORDER_COLOR) != color)
		{
			m_propertiesi[GL_TEXTURE_BORDER_COLOR] = color;

			const color4f border_color = rsx::decode_border_color(color);
			glSamplerParameterfv(samplerHandle, GL_TEXTURE_BORDER_COLOR, border_color.rgba);
		}

		set_parameteri(GL_TEXTURE_WRAP_S, wrap_mode(tex.wrap_s()));
		set_parameteri(GL_TEXTURE_WRAP_T, wrap_mode(tex.wrap_t()));
		set_parameteri(GL_TEXTURE_WRAP_R, wrap_mode(tex.wrap_r()));
		set_parameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		set_parameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		set_parameterf(GL_TEXTURE_LOD_BIAS, tex.bias());
		set_parameterf(GL_TEXTURE_MIN_LOD, tex.min_lod());
		set_parameterf(GL_TEXTURE_MAX_LOD, tex.max_lod());
		set_parameteri(GL_TEXTURE_COMPARE_MODE, GL_NONE);
	}

	void sampler_state::apply_defaults(GLenum default_filter)
	{
		set_parameteri(GL_TEXTURE_WRAP_S, GL_REPEAT);
		set_parameteri(GL_TEXTURE_WRAP_T, GL_REPEAT);
		set_parameteri(GL_TEXTURE_WRAP_R, GL_REPEAT);
		set_parameteri(GL_TEXTURE_MIN_FILTER, default_filter);
		set_parameteri(GL_TEXTURE_MAG_FILTER, default_filter);
		set_parameterf(GL_TEXTURE_LOD_BIAS, 0.f);
		set_parameteri(GL_TEXTURE_MIN_LOD, 0);
		set_parameteri(GL_TEXTURE_MAX_LOD, 0);
		set_parameteri(GL_TEXTURE_COMPARE_MODE, GL_NONE);
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

		case CELL_GCM_TEXTURE_A4R4G4B4:
			return{ GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA };

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

	void* copy_image_to_buffer(const pixel_buffer_layout& pack_info, const gl::texture* src, gl::buffer* dst,
		const int src_level, const coord3u& src_region,  image_memory_requirements* mem_info)
	{
		auto initialize_scratch_mem = [&]()
		{
			const u64 max_mem = (mem_info->memory_required) ? mem_info->memory_required : mem_info->image_size_in_bytes;
			if (!(*dst) || max_mem > static_cast<u64>(dst->size()))
			{
				if (*dst) dst->remove();
				dst->create(buffer::target::pixel_pack, max_mem, nullptr, buffer::memory_type::local, GL_STATIC_COPY);
			}

			dst->bind(buffer::target::pixel_pack);
			src->copy_to(nullptr, static_cast<texture::format>(pack_info.format), static_cast<texture::type>(pack_info.type), src_level, src_region, {});
		};

		void* result = nullptr;
		if (src->aspect() == image_aspect::color ||
			pack_info.type == GL_UNSIGNED_SHORT ||
			pack_info.type == GL_UNSIGNED_INT_24_8)
		{
			initialize_scratch_mem();
			if (auto job = get_trivial_transform_job(pack_info))
			{
				job->run(dst, static_cast<u32>(mem_info->image_size_in_bytes));
			}
		}
		else if (pack_info.type == GL_FLOAT)
		{
			ensure(mem_info->image_size_in_bytes == (mem_info->image_size_in_texels * 4));
			mem_info->memory_required = (mem_info->image_size_in_texels * 6);
			initialize_scratch_mem();

			get_compute_task<cs_fconvert_task<f32, f16, false, true>>()->run(dst, 0,
				static_cast<u32>(mem_info->image_size_in_bytes), static_cast<u32>(mem_info->image_size_in_bytes));
			result = reinterpret_cast<void*>(mem_info->image_size_in_bytes);
		}
		else if (pack_info.type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV)
		{
			ensure(mem_info->image_size_in_bytes == (mem_info->image_size_in_texels * 8));
			mem_info->memory_required = (mem_info->image_size_in_texels * 12);
			initialize_scratch_mem();

			get_compute_task<cs_shuffle_d32fx8_to_x8d24f>()->run(dst, 0,
				static_cast<u32>(mem_info->image_size_in_bytes), static_cast<u32>(mem_info->image_size_in_texels));
			result = reinterpret_cast<void*>(mem_info->image_size_in_bytes);
		}
		else
		{
			fmt::throw_exception("Invalid depth/stencil type 0x%x", pack_info.type);
		}

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_PIXEL_BUFFER_BARRIER_BIT);
		return result;
	}

	void copy_buffer_to_image(const pixel_buffer_layout& unpack_info, gl::buffer* src, gl::texture* dst,
		const void* src_offset, const int dst_level, const coord3u& dst_region, image_memory_requirements* mem_info)
	{
		buffer scratch_mem;
		buffer* transfer_buf = src;
		bool skip_barrier = false;
		u32 in_offset = static_cast<u32>(reinterpret_cast<u64>(src_offset));
		u32 out_offset = in_offset;

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

			scratch_mem.create(buffer::target::pixel_pack, max_mem, nullptr, buffer::memory_type::local, GL_STATIC_COPY);

			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			src->copy_to(&scratch_mem, in_offset, 0, mem_info->image_size_in_bytes);

			in_offset = 0;
			out_offset = static_cast<u32>(mem_info->image_size_in_bytes);
			transfer_buf = &scratch_mem;
		};

		if (dst->aspect() == image_aspect::color ||
			unpack_info.type == GL_UNSIGNED_SHORT ||
			unpack_info.type == GL_UNSIGNED_INT_24_8)
		{
			if (auto job = get_trivial_transform_job(unpack_info))
			{
				job->run(src, static_cast<u32>(mem_info->image_size_in_bytes), in_offset);
			}
			else
			{
				skip_barrier = true;
			}
		}
		else if (unpack_info.type == GL_FLOAT)
		{
			mem_info->memory_required = (mem_info->image_size_in_texels * 4);
			initialize_scratch_mem();
			get_compute_task<cs_fconvert_task<f16, f32, true, false>>()->run(transfer_buf, in_offset, static_cast<u32>(mem_info->image_size_in_bytes), out_offset);
		}
		else if (unpack_info.type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV)
		{
			mem_info->memory_required = (mem_info->image_size_in_texels * 8);
			initialize_scratch_mem();
			get_compute_task<cs_shuffle_x8d24f_to_d32fx8>()->run(transfer_buf, in_offset, out_offset, static_cast<u32>(mem_info->image_size_in_texels));
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

		if (scratch_mem) scratch_mem.remove();
	}

	gl::viewable_image* create_texture(u32 gcm_format, u16 width, u16 height, u16 depth, u16 mipmaps,
			rsx::texture_dimension_extended type)
	{
		if (rsx::is_compressed_host_format(gcm_format))
		{
			//Compressed formats have a 4-byte alignment
			//TODO: Verify that samplers are not affected by the padding
			width = utils::align(width, 4);
			height = utils::align(height, 4);
		}

		const GLenum target = get_target(type);
		const GLenum internal_format = get_sized_internal_format(gcm_format);
		const auto format_class = rsx::classify_format(gcm_format);

		return new gl::viewable_image(target, width, height, depth, mipmaps, internal_format, format_class);
	}

	void fill_texture(texture* dst, int format,
			const std::vector<rsx::subresource_layout> &input_layouts,
			bool is_swizzled, GLenum gl_format, GLenum gl_type, std::vector<std::byte>& staging_buffer)
	{
		rsx::texture_uploader_capabilities caps{ true, false, false, false, 4 };

		pixel_unpack_settings unpack_settings;
		unpack_settings.row_length(0).alignment(4);

		if (rsx::is_compressed_host_format(format)) [[likely]]
		{
			caps.supports_vtc_decoding = gl::get_driver_caps().vendor_NVIDIA;

			unpack_settings.row_length(utils::align(dst->width(), 4));
			unpack_settings.apply();

			glBindTexture(static_cast<GLenum>(dst->get_target()), dst->id());

			const GLsizei format_block_size = (format == CELL_GCM_TEXTURE_COMPRESSED_DXT1) ? 8 : 16;

			for (const rsx::subresource_layout& layout : input_layouts)
			{
				upload_texture_subresource(staging_buffer, layout, format, is_swizzled, caps);
				const sizei image_size{utils::align(layout.width_in_texel, 4), utils::align(layout.height_in_texel, 4)};

				switch (dst->get_target())
				{
				case texture::target::texture1D:
				{
					const GLsizei size = layout.width_in_block * format_block_size;
					glCompressedTexSubImage1D(GL_TEXTURE_1D, layout.level, 0, image_size.width, gl_format, size, staging_buffer.data());
					break;
				}
				case texture::target::texture2D:
				{
					const GLsizei size = layout.width_in_block * layout.height_in_block * format_block_size;
					glCompressedTexSubImage2D(GL_TEXTURE_2D, layout.level, 0, 0, image_size.width, image_size.height, gl_format, size, staging_buffer.data());
					break;
				}
				case texture::target::textureCUBE:
				{
					const GLsizei size = layout.width_in_block * layout.height_in_block * format_block_size;
					glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + layout.layer, layout.level, 0, 0, image_size.width, image_size.height, gl_format, size, staging_buffer.data());
					break;
				}
				case texture::target::texture3D:
				{
					const GLsizei size = layout.width_in_block * layout.height_in_block * layout.depth * format_block_size;
					glCompressedTexSubImage3D(GL_TEXTURE_3D, layout.level, 0, 0, 0, image_size.width, image_size.height, layout.depth, gl_format, size, staging_buffer.data());
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
			bool apply_settings = true;
			bool use_compute_transform = false;
			buffer upload_scratch_mem, compute_scratch_mem;
			image_memory_requirements mem_info;
			pixel_buffer_layout mem_layout;

			std::span<std::byte> dst_buffer = staging_buffer;
			void* out_pointer = staging_buffer.data();
			u8 block_size_in_bytes = rsx::get_format_block_size_in_bytes(format);
			u64 image_linear_size;

			switch (gl_type)
			{
			case GL_BYTE:
			case GL_UNSIGNED_BYTE:
				// Multi-channel format uploaded one byte at a time. This is due to poor driver support for formats like GL_UNSIGNED SHORT_8_8
				// Do byteswapping in software for now until compute acceleration is available
				apply_settings = (gl_format == GL_RED);
				caps.supports_byteswap = apply_settings;
				break;
			case GL_FLOAT:
			case GL_UNSIGNED_INT_24_8:
			case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
				mem_layout.format = gl_format;
				mem_layout.type = gl_type;
				mem_layout.swap_bytes = true;
				mem_layout.size = 4;
				use_compute_transform = true;
				apply_settings = false;
				break;
			}

			if (use_compute_transform)
			{
				upload_scratch_mem.create(staging_buffer.size(), nullptr, buffer::memory_type::host_visible, GL_STREAM_DRAW);
				compute_scratch_mem.create(std::max<GLsizeiptr>(512, staging_buffer.size() * 3), nullptr, buffer::memory_type::local, GL_STATIC_COPY);
				out_pointer = nullptr;
			}

			for (const rsx::subresource_layout& layout : input_layouts)
			{
				if (use_compute_transform)
				{
					const u64 row_pitch = rsx::align2<u64, u64>(layout.width_in_block * block_size_in_bytes, caps.alignment);
					image_linear_size = row_pitch * layout.height_in_block * layout.depth;
					dst_buffer = { reinterpret_cast<std::byte*>(upload_scratch_mem.map(buffer::access::write)), image_linear_size };
				}

				auto op = upload_texture_subresource(dst_buffer, layout, format, is_swizzled, caps);

				// Define upload region
				coord3u region;
				region.x = 0;
				region.y = 0;
				region.z = layout.layer;
				region.width = layout.width_in_texel;
				region.height = layout.height_in_texel;
				region.depth = layout.depth;

				if (use_compute_transform)
				{
					// 1. Unmap buffer
					upload_scratch_mem.unmap();

					// 2. Upload memory to GPU
					upload_scratch_mem.copy_to(&compute_scratch_mem, 0, 0, image_linear_size);

					// 3. Dispatch compute routines
					mem_info.image_size_in_texels = image_linear_size / block_size_in_bytes;
					mem_info.image_size_in_bytes = image_linear_size;
					mem_info.memory_required = 0;
					copy_buffer_to_image(mem_layout, &compute_scratch_mem, dst, nullptr, layout.level, region, & mem_info);
				}
				else
				{
					if (apply_settings)
					{
						unpack_settings.swap_bytes(op.require_swap);
						apply_settings = false;
					}

					dst->copy_from(out_pointer, static_cast<texture::format>(gl_format), static_cast<texture::type>(gl_type), layout.level, region, unpack_settings);
				}
			}

			if (use_compute_transform)
			{
				upload_scratch_mem.remove();
				compute_scratch_mem.remove();
			}
		}
	}

	std::array<GLenum, 4> apply_swizzle_remap(const std::array<GLenum, 4>& swizzle_remap, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& decoded_remap)
	{
		//Remapping tables; format is A-R-G-B
		//Remap input table. Contains channel index to read color from
		const auto remap_inputs = decoded_remap.first;

		//Remap control table. Controls whether the remap value is used, or force either 0 or 1
		const auto remap_lookup = decoded_remap.second;

		std::array<GLenum, 4> remap_values;

		for (u8 channel = 0; channel < 4; ++channel)
		{
			switch (remap_lookup[channel])
			{
			default:
				rsx_log.error("Unknown remap function 0x%X", remap_lookup[channel]);
				[[fallthrough]];
			case CELL_GCM_TEXTURE_REMAP_REMAP:
				remap_values[channel] = swizzle_remap[remap_inputs[channel]];
				break;
			case CELL_GCM_TEXTURE_REMAP_ZERO:
				remap_values[channel] = GL_ZERO;
				break;
			case CELL_GCM_TEXTURE_REMAP_ONE:
				remap_values[channel] = GL_ONE;
				break;
			}
		}

		return remap_values;
	}

	void upload_texture(texture* dst, u32 gcm_format, bool is_swizzled, const std::vector<rsx::subresource_layout>& subresources_layout)
	{
		// Calculate staging buffer size
		const u32 aligned_pitch = utils::align<u32>(dst->pitch(), 4);
		usz texture_data_sz = dst->depth() * dst->height() * aligned_pitch;
		std::vector<std::byte> data_upload_buf(texture_data_sz);

		// TODO: GL drivers support byteswapping and this should be used instead of doing so manually
		const auto format_type = get_format_type(gcm_format);
		const GLenum gl_format = std::get<0>(format_type);
		const GLenum gl_type = std::get<1>(format_type);
		fill_texture(dst, gcm_format, subresources_layout, is_swizzled, gl_format, gl_type, data_upload_buf);
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
		case GL_R8:
		case GL_RG8:
		case GL_RGBA8:
			return { false, 1 };
		case GL_R16:
		case GL_RG16:
		case GL_RG16F:
		case GL_RGB565:
		case GL_RGBA16F:
			return { true, 2 };
		case GL_R32F:
		case GL_RGBA32F:
			return { true, 4 };
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			return { false, 4 };
		case GL_DEPTH_COMPONENT16:
			return { true, 2 };
		case GL_DEPTH24_STENCIL8:
		case GL_DEPTH32F_STENCIL8:
			return { true, 4 };
		default:
			fmt::throw_exception("Unexpected internal format 0x%X", static_cast<u32>(format));
		}
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

	void copy_typeless(texture * dst, const texture * src, const coord3u& dst_region, const coord3u& src_region)
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

			void* data_ptr = copy_image_to_buffer(pack_info, src, &g_typeless_transfer_buffer, 0, src_region, &src_mem);
			copy_buffer_to_image(unpack_info, &g_typeless_transfer_buffer, dst, data_ptr, 0, dst_region, &dst_mem);

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
			if (!g_typeless_transfer_buffer || max_mem > static_cast<u64>(g_typeless_transfer_buffer.size()))
			{
				if (g_typeless_transfer_buffer) g_typeless_transfer_buffer.remove();
				g_typeless_transfer_buffer.create(buffer::target::pixel_pack, max_mem, nullptr, buffer::memory_type::local, GL_STATIC_COPY);
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

			g_typeless_transfer_buffer.bind(buffer::target::pixel_pack);
			src->copy_to(nullptr, static_cast<texture::format>(pack_info.format), static_cast<texture::type>(pack_info.type), 0, src_region, pack_settings);

			glBindBuffer(GL_PIXEL_PACK_BUFFER, GL_NONE);

			// Start unpack operation
			pixel_unpack_settings unpack_settings{};
			unpack_settings.swap_bytes(unpack_info.swap_bytes);

			g_typeless_transfer_buffer.bind(buffer::target::pixel_unpack);
			dst->copy_from(nullptr, static_cast<texture::format>(unpack_info.format), static_cast<texture::type>(unpack_info.type), 0, dst_region, unpack_settings);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
		}
	}

	void copy_typeless(texture* dst, const texture* src)
	{
		const coord3u src_area = { {}, src->size3D() };
		const coord3u dst_area = { {}, dst->size3D() };
		copy_typeless(dst, src, dst_area, src_area);
	}
}
