#include "stdafx.h"
#include "GLTexture.h"
#include "GLHelpers.h"
#include "../GCM.h"
#include "../RSXThread.h"
#include "../RSXTexture.h"
#include "../rsx_utils.h"
#include "../Common/TextureUtils.h"

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
		case CELL_GCM_TEXTURE_DEPTH24_D8: return GL_DEPTH_COMPONENT24;
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
		fmt::throw_exception("Compressed or unknown texture format 0x%x" HERE, texture_format);
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
		}
		fmt::throw_exception("Compressed or unknown texture format 0x%x" HERE, texture_format);
	}
}

namespace
{
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

	bool requires_unpack_byte(u32 texture_format)
	{
		switch (texture_format)
		{
		case CELL_GCM_TEXTURE_R5G6B5:
		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_R5G5B5A1:
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_Y16_X16:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		case CELL_GCM_TEXTURE_D1R5G5B5:
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
			return true;
		}
		return false;
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
			return { GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE };

		case CELL_GCM_TEXTURE_A4R4G4B4:
			return { GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA };

		case CELL_GCM_TEXTURE_B8:
		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_X32_FLOAT:
			return { GL_RED, GL_RED, GL_RED, GL_RED };

		case CELL_GCM_TEXTURE_G8B8: 
			return { GL_GREEN, GL_RED, GL_GREEN, GL_RED};

		case CELL_GCM_TEXTURE_Y16_X16:
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
			return { GL_RED, GL_GREEN, GL_RED, GL_GREEN};

		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
			return { GL_RED, GL_ALPHA, GL_BLUE, GL_GREEN };

		case CELL_GCM_TEXTURE_D1R5G5B5:
		case CELL_GCM_TEXTURE_D8R8G8B8: 
			return { GL_ONE, GL_RED, GL_GREEN, GL_BLUE };

		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8: 
			return { GL_RED, GL_GREEN, GL_RED, GL_GREEN };

		case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			return { GL_ZERO, GL_GREEN, GL_BLUE, GL_RED };

		}
		fmt::throw_exception("Unknown format 0x%x" HERE, texture_format);
	}
}

namespace rsx
{
	namespace gl
	{
		int gl_tex_min_filter(rsx::texture_minify_filter min_filter)
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

		int gl_tex_mag_filter(rsx::texture_magnify_filter mag_filter)
		{
			switch (mag_filter)
			{
			case rsx::texture_magnify_filter::nearest: return GL_NEAREST;
			case rsx::texture_magnify_filter::linear: return GL_LINEAR;
			case rsx::texture_magnify_filter::convolution_mag: return GL_LINEAR;
			}
			fmt::throw_exception("Unknow mag filter" HERE);
		}

		static const int gl_tex_zfunc[] =
		{
			GL_NEVER,
			GL_LESS,
			GL_EQUAL,
			GL_LEQUAL,
			GL_GREATER,
			GL_NOTEQUAL,
			GL_GEQUAL,
			GL_ALWAYS,
		};

		void texture::create()
		{
			if (m_id)
			{
				remove();
			}

			glGenTextures(1, &m_id);
		}

		int texture::gl_wrap(rsx::texture_wrap_mode wrap)
		{
			switch (wrap)
			{
			case rsx::texture_wrap_mode::wrap: return GL_REPEAT;
			case rsx::texture_wrap_mode::mirror: return GL_MIRRORED_REPEAT;
			case rsx::texture_wrap_mode::clamp_to_edge: return GL_CLAMP_TO_EDGE;
			case rsx::texture_wrap_mode::border: return GL_CLAMP_TO_BORDER;
			case rsx::texture_wrap_mode::clamp: return GL_CLAMP_TO_BORDER;
			case rsx::texture_wrap_mode::mirror_once_clamp_to_edge: return GL_MIRROR_CLAMP_TO_EDGE_EXT;
			case rsx::texture_wrap_mode::mirror_once_border: return GL_MIRROR_CLAMP_TO_BORDER_EXT;
			case rsx::texture_wrap_mode::mirror_once_clamp: return GL_MIRROR_CLAMP_EXT;
			}

			LOG_ERROR(RSX, "Texture wrap error: bad wrap (%d)", (u32)wrap);
			return GL_REPEAT;
		}

		float texture::max_aniso(rsx::texture_max_anisotropy aniso)
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

		u16 texture::get_pitch_modifier(u32 format)
		{
			switch (format)
			{
			case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
			case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
			default:
				LOG_ERROR(RSX, "Unimplemented pitch modifier for texture format: 0x%x", format);
				return 0;
			case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
			case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
				return 4;
			case CELL_GCM_TEXTURE_B8:
				return 1;
			case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
			case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
			case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
				return 0;
			case CELL_GCM_TEXTURE_A1R5G5B5:
			case CELL_GCM_TEXTURE_A4R4G4B4:
			case CELL_GCM_TEXTURE_R5G6B5:
			case CELL_GCM_TEXTURE_G8B8:
			case CELL_GCM_TEXTURE_R6G5B5:
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			case CELL_GCM_TEXTURE_X16:
			case CELL_GCM_TEXTURE_R5G5B5A1:
			case CELL_GCM_TEXTURE_D1R5G5B5:
				return 2;
			case CELL_GCM_TEXTURE_A8R8G8B8:
			case CELL_GCM_TEXTURE_X32_FLOAT:
			case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
			case CELL_GCM_TEXTURE_D8R8G8B8:
			case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
			case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
			case CELL_GCM_TEXTURE_Y16_X16:
				return 4;
			case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
				return 8;
			case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
				return 16;
			}
		}

		namespace
		{
			void create_and_fill_texture(rsx::texture_dimension_extended dim,
				u16 mipmap_count, int format, u16 width, u16 height, u16 depth, const std::vector<rsx_subresource_layout> &input_layouts, bool is_swizzled,
				std::vector<gsl::byte> staging_buffer)
			{
				int mip_level = 0;
				if (dim == rsx::texture_dimension_extended::texture_dimension_1d)
				{
					__glcheck glTexStorage1D(GL_TEXTURE_1D, mipmap_count, ::gl::get_sized_internal_format(format), width);
					if (!is_compressed_format(format))
					{
						const auto &format_type = ::gl::get_format_type(format);
						for (const rsx_subresource_layout &layout : input_layouts)
						{
							__glcheck upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
							__glcheck glTexSubImage1D(GL_TEXTURE_1D, mip_level++, 0, layout.width_in_block, std::get<0>(format_type), std::get<1>(format_type), staging_buffer.data());
						}
					}
					else
					{
						for (const rsx_subresource_layout &layout : input_layouts)
						{
							u32 size = layout.width_in_block * ((format == CELL_GCM_TEXTURE_COMPRESSED_DXT1) ? 8 : 16);
							__glcheck upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
							__glcheck glCompressedTexSubImage1D(GL_TEXTURE_1D, mip_level++, 0, layout.width_in_block * 4, ::gl::get_sized_internal_format(format), size, staging_buffer.data());
						}
					}
					return;
				}

				if (dim == rsx::texture_dimension_extended::texture_dimension_2d)
				{
					if (!is_compressed_format(format))
					{
						__glcheck glTexStorage2D(GL_TEXTURE_2D, mipmap_count, ::gl::get_sized_internal_format(format), input_layouts[0].width_in_block, input_layouts[0].height_in_block);
						const auto &format_type = ::gl::get_format_type(format);
						for (const rsx_subresource_layout &layout : input_layouts)
						{
							__glcheck upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
							__glcheck glTexSubImage2D(GL_TEXTURE_2D, mip_level++, 0, 0, layout.width_in_block, layout.height_in_block, std::get<0>(format_type), std::get<1>(format_type), staging_buffer.data());
						}
					}
					else
					{
						__glcheck glTexStorage2D(GL_TEXTURE_2D, mipmap_count, ::gl::get_sized_internal_format(format), input_layouts[0].width_in_block * 4, input_layouts[0].height_in_block * 4);
						for (const rsx_subresource_layout &layout : input_layouts)
						{
							u32 size = layout.width_in_block * layout.height_in_block * ((format == CELL_GCM_TEXTURE_COMPRESSED_DXT1) ? 8 : 16);
							__glcheck upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
							__glcheck glCompressedTexSubImage2D(GL_TEXTURE_2D, mip_level++, 0, 0, layout.width_in_block * 4, layout.height_in_block * 4, ::gl::get_sized_internal_format(format), size, staging_buffer.data());
						}
					}
					return;
				}

				if (dim == rsx::texture_dimension_extended::texture_dimension_cubemap)
				{
					__glcheck glTexStorage2D(GL_TEXTURE_CUBE_MAP, mipmap_count, ::gl::get_sized_internal_format(format), width, height);
					// Note : input_layouts size is get_exact_mipmap_count() for non cubemap texture, and 6 * get_exact_mipmap_count() for cubemap
					// Thus for non cubemap texture, mip_level / mipmap_per_layer will always be rounded to 0.
					// mip_level % mipmap_per_layer will always be equal to mip_level
					if (!is_compressed_format(format))
					{
						const auto &format_type = ::gl::get_format_type(format);
						for (const rsx_subresource_layout &layout : input_layouts)
						{
							upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
							__glcheck glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + mip_level / mipmap_count, mip_level % mipmap_count, 0, 0, layout.width_in_block, layout.height_in_block, std::get<0>(format_type), std::get<1>(format_type), staging_buffer.data());
							mip_level++;
						}
					}
					else
					{
						for (const rsx_subresource_layout &layout : input_layouts)
						{
							u32 size = layout.width_in_block * layout.height_in_block * ((format == CELL_GCM_TEXTURE_COMPRESSED_DXT1) ? 8 : 16);
							__glcheck upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
							__glcheck glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + mip_level / mipmap_count, mip_level % mipmap_count, 0, 0, layout.width_in_block * 4, layout.height_in_block * 4, ::gl::get_sized_internal_format(format), size, staging_buffer.data());
							mip_level++;
						}
					}
					return;
				}

				if (dim == rsx::texture_dimension_extended::texture_dimension_3d)
				{
					__glcheck glTexStorage3D(GL_TEXTURE_3D, mipmap_count, ::gl::get_sized_internal_format(format), width, height, depth);
					if (!is_compressed_format(format))
					{
						const auto &format_type = ::gl::get_format_type(format);
						for (const rsx_subresource_layout &layout : input_layouts)
						{
							__glcheck upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
							__glcheck glTexSubImage3D(GL_TEXTURE_3D, mip_level++, 0, 0, 0, layout.width_in_block, layout.height_in_block, depth, std::get<0>(format_type), std::get<1>(format_type), staging_buffer.data());
						}
					}
					else
					{
						for (const rsx_subresource_layout &layout : input_layouts)
						{
							u32 size = layout.width_in_block * layout.height_in_block * layout.depth * ((format == CELL_GCM_TEXTURE_COMPRESSED_DXT1) ? 8 : 16);
							__glcheck upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
							__glcheck glCompressedTexSubImage3D(GL_TEXTURE_3D, mip_level++, 0, 0, 0, layout.width_in_block * 4, layout.height_in_block * 4, layout.depth, ::gl::get_sized_internal_format(format), size, staging_buffer.data());
						}
					}
					return;
				}
			}
		}

		bool texture::mandates_expansion(u32 format)
		{
			/**
			 * If a texture behaves differently when uploaded directly vs when uploaded via texutils methods, it should be added here.
			 */
			if (format == CELL_GCM_TEXTURE_A1R5G5B5)
				return true;
			
			return false;
		}

		void texture::init(int index, rsx::fragment_texture& tex)
		{
			switch (tex.dimension())
			{
			case rsx::texture_dimension::dimension3d:
				if (!tex.depth())
				{
					return;
				}

			case rsx::texture_dimension::dimension2d:
				if (!tex.height())
				{
					return;
				}

			case rsx::texture_dimension::dimension1d:
				if (!tex.width())
				{
					return;
				}

				break;
			}

			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());

			//We can't re-use texture handles if using immutable storage
			if (m_id)
			{
				__glcheck remove();
			}
			__glcheck create();

			__glcheck glActiveTexture(GL_TEXTURE0 + index);
			bind();

			u32 full_format = tex.format();

			u32 format = full_format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
			bool is_swizzled = !!(~full_format & CELL_GCM_TEXTURE_LN);

			__glcheck ::gl::pixel_pack_settings().apply();
			__glcheck ::gl::pixel_unpack_settings().apply();

			u32 aligned_pitch = tex.pitch();

			size_t texture_data_sz = get_placed_texture_storage_size(tex, 256);
			std::vector<gsl::byte> data_upload_buf(texture_data_sz);
			u32 block_sz = get_pitch_modifier(format);

			__glcheck glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

			__glcheck create_and_fill_texture(tex.get_extended_texture_dimension(), tex.get_exact_mipmap_count(), format, tex.width(), tex.height(), tex.depth(), get_subresources_layout(tex), is_swizzled, data_upload_buf);

			const std::array<GLenum, 4>& glRemap = get_swizzle_remap(format);

			glTexParameteri(m_target, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(m_target, GL_TEXTURE_MAX_LEVEL, tex.get_exact_mipmap_count() - 1);

			auto decoded_remap = tex.decoded_remap();

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
					remap_values[channel] = glRemap[remap_inputs[channel]];
					break;
				case CELL_GCM_TEXTURE_REMAP_ZERO:
					remap_values[channel] = GL_ZERO;
					break;
				case CELL_GCM_TEXTURE_REMAP_ONE:
					remap_values[channel] = GL_ONE;
					break;
				}
			}

			__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_A, remap_values[0]);
			__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_R, remap_values[1]);
			__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_G, remap_values[2]);
			__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_B, remap_values[3]);

			__glcheck glTexParameteri(m_target, GL_TEXTURE_WRAP_S, gl_wrap(tex.wrap_s()));
			__glcheck glTexParameteri(m_target, GL_TEXTURE_WRAP_T, gl_wrap(tex.wrap_t()));
			__glcheck glTexParameteri(m_target, GL_TEXTURE_WRAP_R, gl_wrap(tex.wrap_r()));

			if (tex.get_exact_mipmap_count() <= 1 || m_target == GL_TEXTURE_RECTANGLE)
			{
				GLint min_filter = gl_tex_min_filter(tex.min_filter());
				
				if (min_filter != GL_LINEAR && min_filter != GL_NEAREST)
				{
					LOG_WARNING(RSX, "Texture %d, target 0x%x, requesting mipmap filtering without any mipmaps set!", m_id, m_target);
					
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

				__glcheck glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, min_filter);

				__glcheck glTexParameterf(m_target, GL_TEXTURE_LOD_BIAS, 0.);
				__glcheck glTexParameteri(m_target, GL_TEXTURE_MIN_LOD, 0);
				__glcheck glTexParameteri(m_target, GL_TEXTURE_MAX_LOD, 0);
			}
			else
			{
				__glcheck glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, gl_tex_min_filter(tex.min_filter()));

				__glcheck glTexParameterf(m_target, GL_TEXTURE_LOD_BIAS, tex.bias());
				__glcheck glTexParameteri(m_target, GL_TEXTURE_MIN_LOD, (tex.min_lod() >> 8));
				__glcheck glTexParameteri(m_target, GL_TEXTURE_MAX_LOD, (tex.max_lod() >> 8));
			}

			__glcheck glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, gl_tex_mag_filter(tex.mag_filter()));
			__glcheck glTexParameterf(m_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso(tex.max_aniso()));
		}

		void texture::init(int index, rsx::vertex_texture& tex)
		{
			switch (tex.dimension())
			{
			case rsx::texture_dimension::dimension3d:
				if (!tex.depth())
				{
					return;
				}

			case rsx::texture_dimension::dimension2d:
				if (!tex.height())
				{
					return;
				}

			case rsx::texture_dimension::dimension1d:
				if (!tex.width())
				{
					return;
				}

				break;
			}

			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());

			//We can't re-use texture handles if using immutable storage
			if (m_id)
			{
				__glcheck remove();
			}
			__glcheck create();

			__glcheck glActiveTexture(GL_TEXTURE0 + index);
			bind();

			u32 full_format = tex.format();

			u32 format = full_format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
			bool is_swizzled = !!(~full_format & CELL_GCM_TEXTURE_LN);

			__glcheck::gl::pixel_pack_settings().apply();
			__glcheck::gl::pixel_unpack_settings().apply();

			u32 aligned_pitch = tex.pitch();

			size_t texture_data_sz = get_placed_texture_storage_size(tex, 256);
			std::vector<gsl::byte> data_upload_buf(texture_data_sz);
			u32 block_sz = get_pitch_modifier(format);

			__glcheck glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

			__glcheck create_and_fill_texture(tex.get_extended_texture_dimension(), tex.get_exact_mipmap_count(), format, tex.width(), tex.height(), tex.depth(), get_subresources_layout(tex), is_swizzled, data_upload_buf);

			const std::array<GLenum, 4>& glRemap = get_swizzle_remap(format);

			glTexParameteri(m_target, GL_TEXTURE_MAX_LEVEL, tex.get_exact_mipmap_count() - 1);

			__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_A, glRemap[0]);
			__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_R, glRemap[1]);
			__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_G, glRemap[2]);
			__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_B, glRemap[3]);

			__glcheck glTexParameteri(m_target, GL_TEXTURE_WRAP_S, GL_REPEAT);
			__glcheck glTexParameteri(m_target, GL_TEXTURE_WRAP_T, GL_REPEAT);
			__glcheck glTexParameteri(m_target, GL_TEXTURE_WRAP_R, GL_REPEAT);

			__glcheck glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			__glcheck glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			__glcheck glTexParameterf(m_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.f);
		}

		void texture::bind()
		{
			glBindTexture(m_target, m_id);
		}

		void texture::unbind()
		{
			glBindTexture(m_target, 0);
		}

		void texture::remove()
		{
			if (m_id)
			{
				glDeleteTextures(1, &m_id);
				m_id = 0;
			}
		}

		u32 texture::id() const
		{
			return m_id;
		}
	}
}
