#include "stdafx.h"
#include "GLTexture.h"
#include "GLCompute.h"
#include "../GCM.h"
#include "../RSXThread.h"
#include "../RSXTexture.h"

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
		fmt::throw_exception("Unknown texture target" HERE);
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
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: return GL_DEPTH_COMPONENT32;
		case CELL_GCM_TEXTURE_DEPTH16: return GL_DEPTH_COMPONENT16;
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT: return GL_DEPTH_COMPONENT16;
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
		fmt::throw_exception("Unknown texture format 0x%x" HERE, texture_format);
	}

	std::tuple<GLenum, GLenum> get_format_type(u32 texture_format)
	{
		switch (texture_format)
		{
		case CELL_GCM_TEXTURE_B8: return std::make_tuple(GL_RED, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_A1R5G5B5: return std::make_tuple(GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
		case CELL_GCM_TEXTURE_A4R4G4B4: return std::make_tuple(GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4);
		case CELL_GCM_TEXTURE_R5G6B5: return std::make_tuple(GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
		case CELL_GCM_TEXTURE_A8R8G8B8: return std::make_tuple(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8);
		case CELL_GCM_TEXTURE_G8B8: return std::make_tuple(GL_RG, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_R6G5B5: return std::make_tuple(GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
		case CELL_GCM_TEXTURE_DEPTH24_D8: return std::make_tuple(GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: return std::make_tuple(GL_DEPTH_COMPONENT, GL_FLOAT);
		case CELL_GCM_TEXTURE_DEPTH16: return std::make_tuple(GL_DEPTH_COMPONENT, GL_SHORT);
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT: return std::make_tuple(GL_DEPTH_COMPONENT, GL_HALF_FLOAT);
		case CELL_GCM_TEXTURE_X16: return std::make_tuple(GL_RED, GL_UNSIGNED_SHORT);
		case CELL_GCM_TEXTURE_Y16_X16: return std::make_tuple(GL_RG, GL_UNSIGNED_SHORT);
		case CELL_GCM_TEXTURE_R5G5B5A1: return std::make_tuple(GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1);
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: return std::make_tuple(GL_RGBA, GL_HALF_FLOAT);
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: return std::make_tuple(GL_RGBA, GL_FLOAT);
		case CELL_GCM_TEXTURE_X32_FLOAT: return std::make_tuple(GL_RED, GL_FLOAT);
		case CELL_GCM_TEXTURE_D1R5G5B5: return std::make_tuple(GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
		case CELL_GCM_TEXTURE_D8R8G8B8: return std::make_tuple(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8);
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT: return std::make_tuple(GL_RG, GL_HALF_FLOAT);
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1: return std::make_tuple(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23: return std::make_tuple(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return std::make_tuple(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8: return std::make_tuple(GL_RG, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8: return std::make_tuple(GL_RG, GL_BYTE);
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8: return std::make_tuple(GL_BGRA, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return std::make_tuple(GL_BGRA, GL_UNSIGNED_BYTE);
		}
		fmt::throw_exception("Compressed or unknown texture format 0x%x" HERE, texture_format);
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
			return { GL_RG, GL_UNSIGNED_BYTE, 1, false };
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
		case texture::internal_format::depth24_stencil8:
		case texture::internal_format::depth32f_stencil8:
			return { GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 4, true };
		default:
			fmt::throw_exception("Unexpected internal format 0x%X" HERE, static_cast<u32>(format));
		}
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
		fmt::throw_exception("Unknown min filter" HERE);
	}

	int tex_mag_filter(rsx::texture_magnify_filter mag_filter)
	{
		switch (mag_filter)
		{
		case rsx::texture_magnify_filter::nearest: return GL_NEAREST;
		case rsx::texture_magnify_filter::linear: return GL_LINEAR;
		case rsx::texture_magnify_filter::convolution_mag: return GL_LINEAR;
		}
		fmt::throw_exception("Unknown mag filter" HERE);
	}

	//Apply sampler state settings
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

		const bool aniso_override = !g_cfg.video.strict_rendering_mode && g_cfg.video.anisotropic_level_override > 0;
		f32 af_level = aniso_override ? g_cfg.video.anisotropic_level_override : max_aniso(tex.max_aniso());
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

	bool is_compressed_format(u32 texture_format)
	{
		switch (texture_format)
		{
		case CELL_GCM_TEXTURE_B8:
		case CELL_GCM_TEXTURE_A1R5G5B5:
		case CELL_GCM_TEXTURE_A4R4G4B4:
		case CELL_GCM_TEXTURE_R5G6B5:
		case CELL_GCM_TEXTURE_A8R8G8B8:
		case CELL_GCM_TEXTURE_G8B8:
		case CELL_GCM_TEXTURE_R6G5B5:
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_Y16_X16:
		case CELL_GCM_TEXTURE_R5G5B5A1:
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		case CELL_GCM_TEXTURE_X32_FLOAT:
		case CELL_GCM_TEXTURE_D1R5G5B5:
		case CELL_GCM_TEXTURE_D8R8G8B8:
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			return false;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
			return true;
		}
		fmt::throw_exception("Unknown format 0x%x" HERE, texture_format);
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
			//Blue component is also R (Mass Effect 3)
			return{ GL_RED, GL_ONE, GL_RED, GL_RED };

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
		fmt::throw_exception("Unknown format 0x%x" HERE, texture_format);
	}

	gl::viewable_image* create_texture(u32 gcm_format, u16 width, u16 height, u16 depth, u16 mipmaps,
			rsx::texture_dimension_extended type)
	{
		if (is_compressed_format(gcm_format))
		{
			//Compressed formats have a 4-byte alignment
			//TODO: Verify that samplers are not affected by the padding
			width = align(width, 4);
			height = align(height, 4);
		}

		GLenum target;
		GLenum internal_format = get_sized_internal_format(gcm_format);

		switch (type)
		{
		case rsx::texture_dimension_extended::texture_dimension_1d:
			target = GL_TEXTURE_1D;
			break;
		case rsx::texture_dimension_extended::texture_dimension_2d:
			target = GL_TEXTURE_2D;
			break;
		case rsx::texture_dimension_extended::texture_dimension_3d:
			target = GL_TEXTURE_3D;
			break;
		case rsx::texture_dimension_extended::texture_dimension_cubemap:
			target = GL_TEXTURE_CUBE_MAP;
			break;
		}

		return new gl::viewable_image(target, width, height, depth, mipmaps, internal_format);
	}

	void fill_texture(rsx::texture_dimension_extended dim, u16 mipmap_count, int format, u16 width, u16 height, u16 depth,
			const std::vector<rsx_subresource_layout> &input_layouts, bool is_swizzled, GLenum gl_format, GLenum gl_type, std::vector<std::byte>& staging_buffer)
	{
		texture_uploader_capabilities caps{ true, false, false, 4 };

		pixel_unpack_settings unpack_settings;
		unpack_settings.row_length(0).alignment(4);

		if (is_compressed_format(format)) [[likely]]
		{
			caps.supports_vtc_decoding = gl::get_driver_caps().vendor_NVIDIA;

			unpack_settings.row_length(align(width, 4));
			unpack_settings.apply();

			const GLsizei format_block_size = (format == CELL_GCM_TEXTURE_COMPRESSED_DXT1) ? 8 : 16;

			for (const rsx_subresource_layout& layout : input_layouts)
			{
				upload_texture_subresource(staging_buffer, layout, format, is_swizzled, caps);
				const sizei image_size{ align(layout.width_in_texel, 4), align(layout.height_in_texel, 4) };

				switch (dim)
				{
				case rsx::texture_dimension_extended::texture_dimension_1d:
				{
					const GLsizei size = layout.width_in_block * format_block_size;
					glCompressedTexSubImage1D(GL_TEXTURE_1D, layout.level, 0, image_size.width, gl_format, size, staging_buffer.data());
					break;
				}
				case rsx::texture_dimension_extended::texture_dimension_2d:
				{
					const GLsizei size = layout.width_in_block * layout.height_in_block * format_block_size;
					glCompressedTexSubImage2D(GL_TEXTURE_2D, layout.level, 0, 0, image_size.width, image_size.height, gl_format, size, staging_buffer.data());
					break;
				}
				case rsx::texture_dimension_extended::texture_dimension_cubemap:
				{
					const GLsizei size = layout.width_in_block * layout.height_in_block * format_block_size;
					glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + layout.layer, layout.level, 0, 0, image_size.width, image_size.height, gl_format, size, staging_buffer.data());
					break;
				}
				case rsx::texture_dimension_extended::texture_dimension_3d:
				{
					const GLsizei size = layout.width_in_block * layout.height_in_block * layout.depth * format_block_size;
					glCompressedTexSubImage3D(GL_TEXTURE_3D, layout.level, 0, 0, 0, image_size.width, image_size.height, layout.depth, gl_format, size, staging_buffer.data());
					break;
				}
				default:
				{
					ASSUME(0);
					fmt::throw_exception("Unreachable" HERE);
				}
				}
			}
		}
		else
		{
			bool apply_settings = true;
			switch (gl_type)
			{
			case GL_UNSIGNED_INT_8_8_8_8:
				// NOTE: GL_UNSIGNED_INT_8_8_8_8 is already a swapped type
				// TODO: Remove reliance on format and type checks when compute acceleration is implemented
				apply_settings = false;
				break;
			case GL_BYTE:
			case GL_UNSIGNED_BYTE:
				// Multi-channel format uploaded one byte at a time. This is due to poor driver support for formats like GL_UNSIGNED SHORT_8_8
				// Do byteswapping in software for now until compute acceleration is available
				apply_settings = (gl_format == GL_RED);
				caps.supports_byteswap = apply_settings;
				break;
			default:
				break;
			}

			if (!apply_settings)
			{
				unpack_settings.apply();
			}

			for (const rsx_subresource_layout& layout : input_layouts)
			{
				auto op = upload_texture_subresource(staging_buffer, layout, format, is_swizzled, caps);
				if (apply_settings)
				{
					unpack_settings.swap_bytes(op.require_swap);
					unpack_settings.apply();
					apply_settings = false;
				}

				switch (dim)
				{
				case rsx::texture_dimension_extended::texture_dimension_1d:
					glTexSubImage1D(GL_TEXTURE_1D, layout.level, 0, layout.width_in_texel, gl_format, gl_type, staging_buffer.data());
					break;
				case rsx::texture_dimension_extended::texture_dimension_2d:
					glTexSubImage2D(GL_TEXTURE_2D, layout.level, 0, 0, layout.width_in_texel, layout.height_in_texel, gl_format, gl_type, staging_buffer.data());
					break;
				case rsx::texture_dimension_extended::texture_dimension_cubemap:
					glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + layout.layer, layout.level, 0, 0, layout.width_in_texel, layout.height_in_texel, gl_format, gl_type, staging_buffer.data());
					break;
				case rsx::texture_dimension_extended::texture_dimension_3d:
					glTexSubImage3D(GL_TEXTURE_3D, layout.layer, 0, 0, 0, layout.width_in_texel, layout.height_in_texel, depth, gl_format, gl_type, staging_buffer.data());
					break;
				default:
					ASSUME(0);
					fmt::throw_exception("Unreachable" HERE);
				}
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

	void upload_texture(GLuint id, u32 gcm_format, u16 width, u16 height, u16 depth, u16 mipmaps, bool is_swizzled, rsx::texture_dimension_extended type,
			const std::vector<rsx_subresource_layout>& subresources_layout)
	{
		GLenum target;
		switch (type)
		{
		case rsx::texture_dimension_extended::texture_dimension_1d:
			target = GL_TEXTURE_1D;
			break;
		case rsx::texture_dimension_extended::texture_dimension_2d:
			target = GL_TEXTURE_2D;
			break;
		case rsx::texture_dimension_extended::texture_dimension_3d:
			target = GL_TEXTURE_3D;
			break;
		case rsx::texture_dimension_extended::texture_dimension_cubemap:
			target = GL_TEXTURE_CUBE_MAP;
			break;
		}

		glBindTexture(target, id);
		glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, mipmaps - 1);
		// The rest of sampler state is now handled by sampler state objects

		// Calculate staging buffer size
		const u32 aligned_pitch = align<u32>(width * get_format_block_size_in_bytes(gcm_format), 4);
		size_t texture_data_sz = depth * height * aligned_pitch;
		std::vector<std::byte> data_upload_buf(texture_data_sz);

		// TODO: GL drivers support byteswapping and this should be used instead of doing so manually
		const auto format_type = get_format_type(gcm_format);
		const GLenum gl_format = std::get<0>(format_type);
		const GLenum gl_type = std::get<1>(format_type);
		fill_texture(type, mipmaps, gcm_format, width, height, depth, subresources_layout, is_swizzled, gl_format, gl_type, data_upload_buf);
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
			fmt::throw_exception("Unexpected internal format 0x%X" HERE, static_cast<u32>(format));
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
			fmt::throw_exception("Unexpected internal format 0x%X" HERE, static_cast<u32>(format));
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

	cs_shuffle_base* get_pixel_transform_job(const pixel_buffer_layout& pack_info)
	{
		const bool is_depth_stencil = (pack_info.type == GL_UNSIGNED_INT_24_8);
		if (!is_depth_stencil) [[likely]]
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
				return gl::get_compute_task<gl::cs_shuffle_16>();
				break;
			case 4:
				return gl::get_compute_task<gl::cs_shuffle_32>();
				break;
			default:
				fmt::throw_exception("Unsupported format");
			}
		}
		else
		{
			if (pack_info.swap_bytes)
			{
				return gl::get_compute_task<gl::cs_shuffle_d24x8_to_x8d24<true>>();
			}
			else
			{
				return gl::get_compute_task<gl::cs_shuffle_d24x8_to_x8d24<false>>();
			}
		}
	}

	void copy_typeless(texture * dst, const texture * src, const coord3u& dst_region, const coord3u& src_region)
	{
		const u32 src_mem = src->pitch() * src_region.height;
		const u32 dst_mem = dst->pitch() * dst_region.height;

		auto max_mem = std::max(src_mem, dst_mem);
		if (!g_typeless_transfer_buffer || max_mem > g_typeless_transfer_buffer.size())
		{
			if (g_typeless_transfer_buffer) g_typeless_transfer_buffer.remove();
			g_typeless_transfer_buffer.create(buffer::target::pixel_pack, max_mem, nullptr, buffer::memory_type::local, GL_STATIC_COPY);
		}

		const auto& caps = gl::get_driver_caps();
		const auto pack_info = get_format_type(src->get_internal_format());
		const auto unpack_info = get_format_type(dst->get_internal_format());

		// Start pack operation
		g_typeless_transfer_buffer.bind(buffer::target::pixel_pack);

		if (caps.ARB_compute_shader_supported) [[likely]]
		{
			// Raw copy
			src->copy_to(nullptr, static_cast<texture::format>(pack_info.format), static_cast<texture::type>(pack_info.type), src_region, {});
		}
		else
		{
			pixel_pack_settings pack_settings{};
			pack_settings.swap_bytes(pack_info.swap_bytes);
			src->copy_to(nullptr, static_cast<texture::format>(pack_info.format), static_cast<texture::type>(pack_info.type), src_region, pack_settings);
		}

		glBindBuffer(GL_PIXEL_PACK_BUFFER, GL_NONE);

		// Start unpack operation
		pixel_unpack_settings unpack_settings{};

		if (caps.ARB_compute_shader_supported) [[likely]]
		{
			auto src_transform = get_pixel_transform_job(pack_info);
			auto dst_transform = get_pixel_transform_job(unpack_info);

			if (src->aspect() == gl::image_aspect::color && dst->aspect() == gl::image_aspect::color)
			{
				if (src_transform == dst_transform)
				{
					src_transform = dst_transform = nullptr;
				}
				else if (src_transform && dst_transform)
				{
					src_transform = gl::get_compute_task<cs_shuffle_32_16>();
					dst_transform = nullptr;
				}
			}

			const auto job_length = std::min(src_mem, dst_mem);
			if (src_transform)
			{
				src_transform->run(&g_typeless_transfer_buffer, job_length);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_PIXEL_BUFFER_BARRIER_BIT);
			}

			if (dst_transform)
			{
				dst_transform->run(&g_typeless_transfer_buffer, job_length);
				glMemoryBarrier(GL_PIXEL_BUFFER_BARRIER_BIT);
			}

			// NOTE: glBindBufferRange also binds the buffer to the old-school target.
			// Unbind it to avoid glitching later
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, GL_NONE);
		}
		else
		{
			unpack_settings.swap_bytes(unpack_info.swap_bytes);
		}

		g_typeless_transfer_buffer.bind(buffer::target::pixel_unpack);
		dst->copy_from(nullptr, static_cast<texture::format>(unpack_info.format), static_cast<texture::type>(unpack_info.type), dst_region, unpack_settings);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
	}

	void copy_typeless(texture* dst, const texture* src)
	{
		const coord3u src_area = { {}, src->size3D() };
		const coord3u dst_area = { {}, dst->size3D() };
		copy_typeless(dst, src, dst_area, src_area);
	}
}
