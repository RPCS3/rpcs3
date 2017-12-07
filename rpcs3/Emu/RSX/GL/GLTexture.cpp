#include "stdafx.h"
#include "GLTexture.h"
#include "GLHelpers.h"
#include "../GCM.h"
#include "../RSXThread.h"
#include "../RSXTexture.h"
#include "../rsx_utils.h"

namespace gl
{
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
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: return GL_DEPTH_COMPONENT24;
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
		case CELL_GCM_TEXTURE_R6G5B5: return std::make_tuple(GL_RGBA, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_DEPTH24_D8: return std::make_tuple(GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: return std::make_tuple(GL_DEPTH_COMPONENT, GL_FLOAT);
		case CELL_GCM_TEXTURE_DEPTH16: return std::make_tuple(GL_DEPTH_COMPONENT, GL_SHORT);
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT: return std::make_tuple(GL_DEPTH_COMPONENT, GL_FLOAT);
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
		}
		fmt::throw_exception("Compressed or unknown texture format 0x%x" HERE, texture_format);
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

		LOG_ERROR(RSX, "Texture wrap error: bad wrap (%d)", (u32)wrap);
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

		LOG_ERROR(RSX, "Texture anisotropy error: bad max aniso (%d)", (u32)aniso);
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
		fmt::throw_exception("Unknow min filter" HERE);
	}

	int tex_mag_filter(rsx::texture_magnify_filter mag_filter)
	{
		switch (mag_filter)
		{
		case rsx::texture_magnify_filter::nearest: return GL_NEAREST;
		case rsx::texture_magnify_filter::linear: return GL_LINEAR;
		case rsx::texture_magnify_filter::convolution_mag: return GL_LINEAR;
		}
		fmt::throw_exception("Unknow mag filter" HERE);
	}

	//Apply sampler state settings
	void sampler_state::apply(rsx::fragment_texture& tex)
	{
		const f32 border_color = (f32)tex.border_color() / 255;
		const f32 border_color_array[] = { border_color, border_color, border_color, border_color };

		glSamplerParameteri(samplerHandle, GL_TEXTURE_WRAP_S, wrap_mode(tex.wrap_s()));
		glSamplerParameteri(samplerHandle, GL_TEXTURE_WRAP_T, wrap_mode(tex.wrap_t()));
		glSamplerParameteri(samplerHandle, GL_TEXTURE_WRAP_R, wrap_mode(tex.wrap_r()));
		glSamplerParameterfv(samplerHandle, GL_TEXTURE_BORDER_COLOR, border_color_array);

		if (tex.get_exact_mipmap_count() <= 1)
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
					LOG_ERROR(RSX, "No mipmap fallback defined for rsx_min_filter = 0x%X", (u32)tex.min_filter());
					min_filter = GL_NEAREST;
				}
			}

			glSamplerParameteri(samplerHandle, GL_TEXTURE_MIN_FILTER, min_filter);
			glSamplerParameterf(samplerHandle, GL_TEXTURE_LOD_BIAS, 0.f);
			glSamplerParameterf(samplerHandle, GL_TEXTURE_MIN_LOD, -1000.f);
			glSamplerParameterf(samplerHandle, GL_TEXTURE_MAX_LOD, 1000.f);
		}
		else
		{
			glSamplerParameteri(samplerHandle, GL_TEXTURE_MIN_FILTER, tex_min_filter(tex.min_filter()));
			glSamplerParameterf(samplerHandle, GL_TEXTURE_LOD_BIAS, tex.bias());
			glSamplerParameteri(samplerHandle, GL_TEXTURE_MIN_LOD, (tex.min_lod() >> 8));
			glSamplerParameteri(samplerHandle, GL_TEXTURE_MAX_LOD, (tex.max_lod() >> 8));
		}

		f32 af_level = g_cfg.video.anisotropic_level_override > 0 ? (f32)g_cfg.video.anisotropic_level_override : max_aniso(tex.max_aniso());
		glSamplerParameterf(samplerHandle, GL_TEXTURE_MAX_ANISOTROPY_EXT, af_level);
		glSamplerParameteri(samplerHandle, GL_TEXTURE_MAG_FILTER, tex_mag_filter(tex.mag_filter()));

		const u32 texture_format = tex.format() & ~(CELL_GCM_TEXTURE_UN | CELL_GCM_TEXTURE_LN);
		if (texture_format == CELL_GCM_TEXTURE_DEPTH16 || texture_format == CELL_GCM_TEXTURE_DEPTH24_D8)
		{
			//NOTE: The stored texture function is reversed wrt the textureProj compare function
			GLenum compare_mode = (GLenum)tex.zfunc() | GL_NEVER;

			switch (compare_mode)
			{
			case GL_GREATER: compare_mode = GL_LESS; break;
			case GL_GEQUAL: compare_mode = GL_LEQUAL; break;
			case GL_LESS: compare_mode = GL_GREATER; break;
			case GL_LEQUAL: compare_mode = GL_GEQUAL; break;
			}

			glSamplerParameteri(samplerHandle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			glSamplerParameteri(samplerHandle, GL_TEXTURE_COMPARE_FUNC, compare_mode);
		}
		else
			glSamplerParameteri(samplerHandle, GL_TEXTURE_COMPARE_MODE, GL_NONE);
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
		case CELL_GCM_TEXTURE_A8R8G8B8: // TODO
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			return{ GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE };

		case CELL_GCM_TEXTURE_A4R4G4B4:
			return{ GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA };

		case CELL_GCM_TEXTURE_B8:
		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_X32_FLOAT:
			return{ GL_RED, GL_RED, GL_RED, GL_RED };

		case CELL_GCM_TEXTURE_G8B8:
			return{ GL_GREEN, GL_RED, GL_GREEN, GL_RED };

		case CELL_GCM_TEXTURE_Y16_X16:
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
			return{ GL_RED, GL_GREEN, GL_RED, GL_GREEN };

		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
			return{ GL_RED, GL_ALPHA, GL_BLUE, GL_GREEN };

		case CELL_GCM_TEXTURE_D1R5G5B5:
		case CELL_GCM_TEXTURE_D8R8G8B8:
			return{ GL_ONE, GL_RED, GL_GREEN, GL_BLUE };

		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
			return{ GL_RED, GL_GREEN, GL_RED, GL_GREEN };

		case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			return{ GL_ZERO, GL_GREEN, GL_BLUE, GL_RED };

		}
		fmt::throw_exception("Unknown format 0x%x" HERE, texture_format);
	}

	GLuint create_texture(u32 gcm_format, u16 width, u16 height, u16 depth, u16 mipmaps, rsx::texture_dimension_extended type)
	{
		if (is_compressed_format(gcm_format))
		{
			//Compressed formats have a 4-byte alignment
			//TODO: Verify that samplers are not affected by the padding
			width = align(width, 4);
			height = align(height, 4);
		}

		GLuint id = 0;
		GLenum target;
		GLenum internal_format = get_sized_internal_format(gcm_format);

		glGenTextures(1, &id);
		
		switch (type)
		{
		case rsx::texture_dimension_extended::texture_dimension_1d:
			target = GL_TEXTURE_1D;
			glBindTexture(GL_TEXTURE_1D, id);
			glTexStorage1D(GL_TEXTURE_1D, mipmaps, internal_format, width);
			break;
		case rsx::texture_dimension_extended::texture_dimension_2d:
			target = GL_TEXTURE_2D;
			glBindTexture(GL_TEXTURE_2D, id);
			glTexStorage2D(GL_TEXTURE_2D, mipmaps, internal_format, width, height);
			break;
		case rsx::texture_dimension_extended::texture_dimension_3d:
			target = GL_TEXTURE_3D;
			glBindTexture(GL_TEXTURE_3D, id);
			glTexStorage3D(GL_TEXTURE_3D, mipmaps, internal_format, width, height, depth);
			break;
		case rsx::texture_dimension_extended::texture_dimension_cubemap:
			target = GL_TEXTURE_CUBE_MAP;
			glBindTexture(GL_TEXTURE_CUBE_MAP, id);
			glTexStorage2D(GL_TEXTURE_CUBE_MAP, mipmaps, internal_format, width, height);
			break;
		}

		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		return id;
	}

	void fill_texture(rsx::texture_dimension_extended dim, u16 mipmap_count, int format, u16 width, u16 height, u16 depth,
			const std::vector<rsx_subresource_layout> &input_layouts, bool is_swizzled, GLenum gl_format, GLenum gl_type, std::vector<gsl::byte> staging_buffer)
	{
		int mip_level = 0;
		if (is_compressed_format(format))
		{
			//Compressed formats have a 4-byte alignment
			//TODO: Verify that samplers are not affected by the padding
			width = align(width, 4);
			height = align(height, 4);
		}

		if (dim == rsx::texture_dimension_extended::texture_dimension_1d)
		{
			if (!is_compressed_format(format))
			{
				for (const rsx_subresource_layout &layout : input_layouts)
				{
					upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
					glTexSubImage1D(GL_TEXTURE_1D, mip_level++, 0, layout.width_in_block, gl_format, gl_type, staging_buffer.data());
				}
			}
			else
			{
				for (const rsx_subresource_layout &layout : input_layouts)
				{
					u32 size = layout.width_in_block * ((format == CELL_GCM_TEXTURE_COMPRESSED_DXT1) ? 8 : 16);
					upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
					glCompressedTexSubImage1D(GL_TEXTURE_1D, mip_level++, 0, layout.width_in_block * 4, gl_format, size, staging_buffer.data());
				}
			}
			return;
		}

		if (dim == rsx::texture_dimension_extended::texture_dimension_2d)
		{
			if (!is_compressed_format(format))
			{
				for (const rsx_subresource_layout &layout : input_layouts)
				{
					upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
					glTexSubImage2D(GL_TEXTURE_2D, mip_level++, 0, 0, layout.width_in_block, layout.height_in_block, gl_format, gl_type, staging_buffer.data());
				}
			}
			else
			{
				for (const rsx_subresource_layout &layout : input_layouts)
				{
					u32 size = layout.width_in_block * layout.height_in_block * ((format == CELL_GCM_TEXTURE_COMPRESSED_DXT1) ? 8 : 16);
					upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
					glCompressedTexSubImage2D(GL_TEXTURE_2D, mip_level++, 0, 0, layout.width_in_block * 4, layout.height_in_block * 4, gl_format, size, staging_buffer.data());
				}
			}
			return;
		}

		if (dim == rsx::texture_dimension_extended::texture_dimension_cubemap)
		{
			// Note : input_layouts size is get_exact_mipmap_count() for non cubemap texture, and 6 * get_exact_mipmap_count() for cubemap
			// Thus for non cubemap texture, mip_level / mipmap_per_layer will always be rounded to 0.
			// mip_level % mipmap_per_layer will always be equal to mip_level
			if (!is_compressed_format(format))
			{
				for (const rsx_subresource_layout &layout : input_layouts)
				{
					upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
					glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + mip_level / mipmap_count, mip_level % mipmap_count, 0, 0, layout.width_in_block, layout.height_in_block, gl_format, gl_type, staging_buffer.data());
					mip_level++;
				}
			}
			else
			{
				for (const rsx_subresource_layout &layout : input_layouts)
				{
					u32 size = layout.width_in_block * layout.height_in_block * ((format == CELL_GCM_TEXTURE_COMPRESSED_DXT1) ? 8 : 16);
					upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
					glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + mip_level / mipmap_count, mip_level % mipmap_count, 0, 0, layout.width_in_block * 4, layout.height_in_block * 4, gl_format, size, staging_buffer.data());
					mip_level++;
				}
			}
			return;
		}

		if (dim == rsx::texture_dimension_extended::texture_dimension_3d)
		{
			if (!is_compressed_format(format))
			{
				for (const rsx_subresource_layout &layout : input_layouts)
				{
					upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
					glTexSubImage3D(GL_TEXTURE_3D, mip_level++, 0, 0, 0, layout.width_in_block, layout.height_in_block, depth, gl_format, gl_type, staging_buffer.data());
				}
			}
			else
			{
				for (const rsx_subresource_layout &layout : input_layouts)
				{
					u32 size = layout.width_in_block * layout.height_in_block * layout.depth * ((format == CELL_GCM_TEXTURE_COMPRESSED_DXT1) ? 8 : 16);
					upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
					glCompressedTexSubImage3D(GL_TEXTURE_3D, mip_level++, 0, 0, 0, layout.width_in_block * 4, layout.height_in_block * 4, layout.depth, gl_format, size, staging_buffer.data());
				}
			}
			return;
		}
	}

	void apply_swizzle_remap(const GLenum target, const std::array<GLenum, 4>& swizzle_remap, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& decoded_remap)
	{
		//Remapping tables; format is A-R-G-B
		//Remap input table. Contains channel index to read color from
		const auto remap_inputs = decoded_remap.first;

		//Remap control table. Controls whether the remap value is used, or force either 0 or 1
		const auto remap_lookup = decoded_remap.second;

		GLenum remap_values[4];

		for (u8 channel = 0; channel < 4; ++channel)
		{
			switch (remap_lookup[channel])
			{
			default:
				LOG_ERROR(RSX, "Unknown remap function 0x%X", remap_lookup[channel]);
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

		glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, remap_values[0]);
		glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, remap_values[1]);
		glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, remap_values[2]);
		glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, remap_values[3]);
	}

	void upload_texture(const GLuint id, const u32 texaddr, const u32 gcm_format, u16 width, u16 height, u16 depth, u16 mipmaps, bool is_swizzled, rsx::texture_dimension_extended type,
			std::vector<rsx_subresource_layout>& subresources_layout, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& decoded_remap, bool static_state)
	{
		const bool is_cubemap = type == rsx::texture_dimension_extended::texture_dimension_cubemap;
		
		size_t texture_data_sz = get_placed_texture_storage_size(width, height, depth, gcm_format, mipmaps, is_cubemap, 256, 512);
		std::vector<gsl::byte> data_upload_buf(texture_data_sz);

		const std::array<GLenum, 4>& glRemap = get_swizzle_remap(gcm_format);

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
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, mipmaps - 1);

		if (static_state)
		{
			//Usually for vertex textures

			glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, glRemap[0]);
			glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, glRemap[1]);
			glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, glRemap[2]);
			glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, glRemap[3]);

			glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_REPEAT);

			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.f);
		}
		else
		{
			apply_swizzle_remap(target, glRemap, decoded_remap);
		}

		//The rest of sampler state is now handled by sampler state objects

		const auto format_type = get_format_type(gcm_format);
		const GLenum gl_format = std::get<0>(format_type);
		const GLenum gl_type = std::get<1>(format_type);
		fill_texture(type, mipmaps, gcm_format, width, height, depth, subresources_layout, is_swizzled, gl_format, gl_type, data_upload_buf);
	}
}
