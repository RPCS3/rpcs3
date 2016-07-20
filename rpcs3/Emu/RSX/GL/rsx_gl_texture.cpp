#include "stdafx.h"
#include "rsx_gl_texture.h"
#include "gl_helpers.h"
#include "../GCM.h"
#include "../RSXThread.h"
#include "../RSXTexture.h"
#include "../rsx_utils.h"
#include "../Common/TextureUtils.h"

namespace
{
	GLenum get_sized_internal_format(rsx::texture::format texture_format)
	{
		switch (texture_format)
		{
		case rsx::texture::format::b8: return GL_R8;
		case rsx::texture::format::a1r5g5b5: return GL_RGB5_A1;
		case rsx::texture::format::a4r4g4b4: return GL_RGBA4;
		case rsx::texture::format::r5g6b5: return GL_RGB565;
		case rsx::texture::format::a8r8g8b8: return GL_RGBA8;
		case rsx::texture::format::g8b8: return GL_RG8;
		case rsx::texture::format::r6g5b5: return GL_RGB565;
		case rsx::texture::format::d24_8: return GL_DEPTH_COMPONENT24;
		case rsx::texture::format::d24_8_float: return GL_DEPTH_COMPONENT24;
		case rsx::texture::format::d16: return GL_DEPTH_COMPONENT16;
		case rsx::texture::format::d16_float: return GL_DEPTH_COMPONENT16;
		case rsx::texture::format::x16: return GL_R16;
		case rsx::texture::format::y16x16: return GL_RG16;
		case rsx::texture::format::r5g5b5a1: return GL_RGB5_A1;
		case rsx::texture::format::w16z16y16x16_float: return GL_RGBA16F;
		case rsx::texture::format::w32z32y32x32_float: return GL_RGBA32F;
		case rsx::texture::format::x32float: return GL_R32F;
		case rsx::texture::format::d1r5g5b5: return GL_RGB5_A1;
		case rsx::texture::format::d8r8g8b8: return GL_RGBA8;
		case rsx::texture::format::y16x16_float: return GL_RG16F;
		case rsx::texture::format::compressed_dxt1: return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		case rsx::texture::format::compressed_dxt23: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case rsx::texture::format::compressed_dxt45: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		}
		throw EXCEPTION("Compressed or unknown texture format %x", texture_format);
	}


	std::tuple<GLenum, GLenum> get_format_type(rsx::texture::format texture_format)
	{
		switch (texture_format)
		{
		case rsx::texture::format::b8: return std::make_tuple(GL_RED, GL_UNSIGNED_BYTE);
		case rsx::texture::format::a1r5g5b5: return std::make_tuple(GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
		case rsx::texture::format::a4r4g4b4: return std::make_tuple(GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4);
		case rsx::texture::format::r5g6b5: return std::make_tuple(GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
		case rsx::texture::format::a8r8g8b8: return std::make_tuple(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8);
		case rsx::texture::format::g8b8: return std::make_tuple(GL_RG, GL_UNSIGNED_BYTE);
		case rsx::texture::format::r6g5b5: return std::make_tuple(GL_RGBA, GL_UNSIGNED_BYTE);
		case rsx::texture::format::d24_8: return std::make_tuple(GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE);
		case rsx::texture::format::d24_8_float: return std::make_tuple(GL_DEPTH_COMPONENT, GL_FLOAT);
		case rsx::texture::format::d16: return std::make_tuple(GL_DEPTH_COMPONENT, GL_SHORT);
		case rsx::texture::format::d16_float: return std::make_tuple(GL_DEPTH_COMPONENT, GL_FLOAT);
		case rsx::texture::format::x16: return std::make_tuple(GL_RED, GL_UNSIGNED_SHORT);
		case rsx::texture::format::y16x16: return std::make_tuple(GL_RG, GL_UNSIGNED_SHORT);
		case rsx::texture::format::r5g5b5a1: return std::make_tuple(GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1);
		case rsx::texture::format::w16z16y16x16_float: return std::make_tuple(GL_RGBA, GL_HALF_FLOAT);
		case rsx::texture::format::w32z32y32x32_float: return std::make_tuple(GL_RGBA, GL_FLOAT);
		case rsx::texture::format::x32float: return std::make_tuple(GL_RED, GL_FLOAT);
		case rsx::texture::format::d1r5g5b5: return std::make_tuple(GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
		case rsx::texture::format::d8r8g8b8: return std::make_tuple(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8);
		case rsx::texture::format::y16x16_float: return std::make_tuple(GL_RG, GL_HALF_FLOAT);
		}
		throw EXCEPTION("Compressed or unknown texture format %x", texture_format);
	}

	bool is_compressed_format(rsx::texture::format texture_format)
	{
		switch (texture_format)
		{
		case rsx::texture::format::b8:
		case rsx::texture::format::a1r5g5b5:
		case rsx::texture::format::a4r4g4b4:
		case rsx::texture::format::r5g6b5:
		case rsx::texture::format::a8r8g8b8:
		case rsx::texture::format::g8b8:
		case rsx::texture::format::r6g5b5:
		case rsx::texture::format::d24_8:
		case rsx::texture::format::d24_8_float:
		case rsx::texture::format::d16:
		case rsx::texture::format::d16_float:
		case rsx::texture::format::x16:
		case rsx::texture::format::y16x16:
		case rsx::texture::format::r5g5b5a1:
		case rsx::texture::format::w16z16y16x16_float:
		case rsx::texture::format::w32z32y32x32_float:
		case rsx::texture::format::x32float:
		case rsx::texture::format::d1r5g5b5:
		case rsx::texture::format::d8r8g8b8:
		case rsx::texture::format::y16x16_float:
			return false;
		case rsx::texture::format::compressed_dxt1:
		case rsx::texture::format::compressed_dxt23:
		case rsx::texture::format::compressed_dxt45:
			return true;
		}
		throw EXCEPTION("Unknown format %x", texture_format);
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

	std::array<GLenum, 4> get_swizzle_remap(rsx::texture::format texture_format)
	{
		// NOTE: This must be in ARGB order in all forms below.
		switch (texture_format)
		{
		case rsx::texture::format::a1r5g5b5:
		case rsx::texture::format::r5g5b5a1:
		case rsx::texture::format::r6g5b5:
		case rsx::texture::format::r5g6b5:
		case rsx::texture::format::a8r8g8b8: // TODO
		case rsx::texture::format::d24_8:
		case rsx::texture::format::d24_8_float:
		case rsx::texture::format::d16:
		case rsx::texture::format::d16_float:
		case rsx::texture::format::w32z32y32x32_float:
		case rsx::texture::format::compressed_dxt1:
		case rsx::texture::format::compressed_dxt23:
		case rsx::texture::format::compressed_dxt45:
		case rsx::texture::format::compressed_b8r8_g8r8:
		case rsx::texture::format::compressed_r8b8_r8g8:
			return { GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE };

		case rsx::texture::format::b8:
			return { GL_RED, GL_RED, GL_RED, GL_RED };

		case rsx::texture::format::a4r4g4b4:
			return { GL_BLUE, GL_ALPHA, GL_RED, GL_GREEN };

		case rsx::texture::format::g8b8:
			return { GL_GREEN, GL_RED, GL_GREEN, GL_RED};

		case rsx::texture::format::x16:
			return { GL_RED, GL_ONE, GL_RED, GL_ONE };

		case rsx::texture::format::y16x16:
			return { GL_RED, GL_GREEN, GL_RED, GL_GREEN};

		case rsx::texture::format::x32float:
			return { GL_RED, GL_ONE, GL_ONE, GL_ONE };

		case rsx::texture::format::y16x16_float:
			return { GL_GREEN, GL_RED, GL_GREEN, GL_RED };

		case rsx::texture::format::w16z16y16x16_float:
			return { GL_RED, GL_ALPHA, GL_BLUE, GL_GREEN };

		case rsx::texture::format::d1r5g5b5:
		case rsx::texture::format::d8r8g8b8:
			return { GL_ONE, GL_RED, GL_GREEN, GL_BLUE };

		case rsx::texture::format::compressed_hilo_8:
		case rsx::texture::format::compressed_hilo_s8:
			return { GL_RED, GL_GREEN, GL_RED, GL_GREEN };
		}
		throw EXCEPTION("Unknown format %x", texture_format);
	}
}

namespace rsx
{
	namespace gl
	{
		int gl_tex_min_filter(rsx::texture::minify_filter min_filter)
		{
			switch (min_filter)
			{
			case rsx::texture::minify_filter::nearest: return GL_NEAREST;
			case rsx::texture::minify_filter::linear: return GL_LINEAR;
			case rsx::texture::minify_filter::nearest_nearest: return GL_NEAREST_MIPMAP_NEAREST;
			case rsx::texture::minify_filter::linear_nearest: return GL_LINEAR_MIPMAP_NEAREST;
			case rsx::texture::minify_filter::nearest_linear: return GL_NEAREST_MIPMAP_LINEAR;
			case rsx::texture::minify_filter::linear_linear: return GL_LINEAR_MIPMAP_LINEAR;
			case rsx::texture::minify_filter::convolution_min: return GL_LINEAR_MIPMAP_LINEAR;
			}
			throw EXCEPTION("Unknow min filter");
		}

		int gl_tex_mag_filter(rsx::texture::magnify_filter mag_filter)
		{
			switch (mag_filter)
			{
			case rsx::texture::magnify_filter::nearest: return GL_NEAREST;
			case rsx::texture::magnify_filter::linear: return GL_LINEAR;
			case rsx::texture::magnify_filter::convolution_mag: return GL_LINEAR;
			}
			throw EXCEPTION("Unknow mag filter");
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

		int texture::gl_wrap(rsx::texture::wrap_mode wrap)
		{
			switch (wrap)
			{
			case rsx::texture::wrap_mode::wrap: return GL_REPEAT;
			case rsx::texture::wrap_mode::mirror: return GL_MIRRORED_REPEAT;
			case rsx::texture::wrap_mode::clamp_to_edge: return GL_CLAMP_TO_EDGE;
			case rsx::texture::wrap_mode::border: return GL_CLAMP_TO_BORDER;
			case rsx::texture::wrap_mode::clamp: return GL_CLAMP_TO_BORDER;
			case rsx::texture::wrap_mode::mirror_once_clamp_to_edge: return GL_MIRROR_CLAMP_TO_EDGE_EXT;
			case rsx::texture::wrap_mode::mirror_once_border: return GL_MIRROR_CLAMP_TO_BORDER_EXT;
			case rsx::texture::wrap_mode::mirror_once_clamp: return GL_MIRROR_CLAMP_EXT;
			}

			LOG_ERROR(RSX, "Texture wrap error: bad wrap (%d).", wrap);
			return GL_REPEAT;
		}

		float texture::max_aniso(rsx::texture::max_anisotropy aniso)
		{
			switch (aniso)
			{
			case rsx::texture::max_anisotropy::x1: return 1.0f;
			case rsx::texture::max_anisotropy::x2: return 2.0f;
			case rsx::texture::max_anisotropy::x4: return 4.0f;
			case rsx::texture::max_anisotropy::x6: return 6.0f;
			case rsx::texture::max_anisotropy::x8: return 8.0f;
			case rsx::texture::max_anisotropy::x10: return 10.0f;
			case rsx::texture::max_anisotropy::x12: return 12.0f;
			case rsx::texture::max_anisotropy::x16: return 16.0f;
			}

			LOG_ERROR(RSX, "Texture anisotropy error: bad max aniso (%d).", aniso);
			return 1.0f;
		}

		u16 texture::get_pitch_modifier(rsx::texture::format format)
		{
			switch (format)
			{
			case rsx::texture::format::compressed_hilo_8:
			case rsx::texture::format::compressed_hilo_s8:
			default:
				LOG_ERROR(RSX, "Unimplemented pitch modifier for texture format: 0x%x", format);
				return 0;
			case rsx::texture::format::b8:
				return 1;
			case rsx::texture::format::compressed_dxt1:
			case rsx::texture::format::compressed_dxt23:
			case rsx::texture::format::compressed_dxt45:
				return 0;
			case rsx::texture::format::a1r5g5b5:
			case rsx::texture::format::a4r4g4b4:
			case rsx::texture::format::r5g6b5:
			case rsx::texture::format::g8b8:
			case rsx::texture::format::r6g5b5:
			case rsx::texture::format::d16:
			case rsx::texture::format::d16_float:
			case rsx::texture::format::x16:
			case rsx::texture::format::r5g5b5a1:
			case rsx::texture::format::d1r5g5b5:
				return 2;
			case rsx::texture::format::a8r8g8b8:
			case rsx::texture::format::x32float:
			case rsx::texture::format::y16x16_float:
			case rsx::texture::format::d8r8g8b8:
			case rsx::texture::format::compressed_r8b8_r8g8:
			case rsx::texture::format::compressed_b8r8_g8r8:
			case rsx::texture::format::d24_8:
			case rsx::texture::format::d24_8_float:
			case rsx::texture::format::y16x16:
				return 4;
			case rsx::texture::format::w16z16y16x16_float:
				return 8;
			case rsx::texture::format::w32z32y32x32_float:
				return 16;
			}
		}

		namespace
		{
			void create_and_fill_texture(rsx::texture_dimension_extended dim,
				u16 mipmap_count, rsx::texture::format format, u16 width, u16 height, u16 depth, const std::vector<rsx_subresource_layout> &input_layouts, bool is_swizzled,
				std::vector<gsl::byte> staging_buffer)
			{
				int mip_level = 0;
				if (dim == rsx::texture_dimension_extended::texture_dimension_1d)
				{
					__glcheck glTexStorage1D(GL_TEXTURE_1D, mipmap_count, get_sized_internal_format(format), width);
					if (!is_compressed_format(format))
					{
						const auto &format_type = get_format_type(format);
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
							u32 size = layout.width_in_block * ((format == rsx::texture::format::compressed_dxt1) ? 8 : 16);
							__glcheck glCompressedTexSubImage1D(GL_TEXTURE_1D, mip_level++, 0, layout.width_in_block * 4, get_sized_internal_format(format), size, layout.data.data());
						}
					}
					return;
				}

				if (dim == rsx::texture_dimension_extended::texture_dimension_2d)
				{
					__glcheck glTexStorage2D(GL_TEXTURE_2D, mipmap_count, get_sized_internal_format(format), width, height);
					if (!is_compressed_format(format))
					{
						const auto &format_type = get_format_type(format);
						for (const rsx_subresource_layout &layout : input_layouts)
						{
							__glcheck upload_texture_subresource(staging_buffer, layout, format, is_swizzled, 4);
							__glcheck glTexSubImage2D(GL_TEXTURE_2D, mip_level++, 0, 0, layout.width_in_block, layout.height_in_block, std::get<0>(format_type), std::get<1>(format_type), staging_buffer.data());
						}
					}
					else
					{
						for (const rsx_subresource_layout &layout : input_layouts)
						{
							u32 size = layout.width_in_block * layout.height_in_block * ((format == rsx::texture::format::compressed_dxt1) ? 8 : 16);
							__glcheck glCompressedTexSubImage2D(GL_TEXTURE_2D, mip_level++, 0, 0, layout.width_in_block * 4, layout.height_in_block * 4, get_sized_internal_format(format), size, layout.data.data());
						}
					}
					return;
				}

				if (dim == rsx::texture_dimension_extended::texture_dimension_cubemap)
				{
					__glcheck glTexStorage2D(GL_TEXTURE_CUBE_MAP, mipmap_count, get_sized_internal_format(format), width, height);
					// Note : input_layouts size is get_exact_mipmap_count() for non cubemap texture, and 6 * get_exact_mipmap_count() for cubemap
					// Thus for non cubemap texture, mip_level / mipmap_per_layer will always be rounded to 0.
					// mip_level % mipmap_per_layer will always be equal to mip_level
					if (!is_compressed_format(format))
					{
						const auto &format_type = get_format_type(format);
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
							u32 size = layout.width_in_block * layout.height_in_block * ((format == rsx::texture::format::compressed_dxt1) ? 8 : 16);
							__glcheck glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + mip_level / mipmap_count, mip_level % mipmap_count, 0, 0, layout.width_in_block * 4, layout.height_in_block * 4, get_sized_internal_format(format), size, layout.data.data());
							mip_level++;
						}
					}
					return;
				}

				if (dim == rsx::texture_dimension_extended::texture_dimension_3d)
				{
					__glcheck glTexStorage3D(GL_TEXTURE_3D, mipmap_count, get_sized_internal_format(format), width, height, depth);
					if (!is_compressed_format(format))
					{
						const auto &format_type = get_format_type(format);
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
							u32 size = layout.width_in_block * layout.height_in_block * layout.depth * ((format == rsx::texture::format::compressed_dxt1) ? 8 : 16);
							__glcheck glCompressedTexSubImage3D(GL_TEXTURE_3D, mip_level++, 0, 0, 0, layout.width_in_block * 4, layout.height_in_block * 4, layout.depth, get_sized_internal_format(format), size, layout.data.data());
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

		void texture::init(int index, rsx::texture_t& tex)
		{
			switch (tex.dimension())
			{
			case rsx::texture::dimension::dimension3d:
				if (!tex.depth())
				{
					return;
				}

			case rsx::texture::dimension::dimension2d:
				if (!tex.height())
				{
					return;
				}

			case rsx::texture::dimension::dimension1d:
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

			rsx::texture::format format = tex.format();
			bool is_swizzled = tex.layout() == rsx::texture::layout::swizzled;

			__glcheck ::gl::pixel_pack_settings().apply();
			__glcheck ::gl::pixel_unpack_settings().apply();

			u32 aligned_pitch = tex.pitch();

			size_t texture_data_sz = get_placed_texture_storage_size(tex, 256);
			std::vector<gsl::byte> data_upload_buf(texture_data_sz);
			u32 block_sz = get_pitch_modifier(format);

			__glcheck glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

			__glcheck create_and_fill_texture(tex.get_extended_texture_dimension(), tex.get_exact_mipmap_count(), format, tex.width(), tex.height(), tex.depth(), get_subresources_layout(tex), is_swizzled, data_upload_buf);

			const std::array<GLenum, 4>& glRemap = get_swizzle_remap(format);

			glTexParameteri(m_target, GL_TEXTURE_MAX_LEVEL, tex.get_exact_mipmap_count() - 1);

			if (format != rsx::texture::format::b8 && format != rsx::texture::format::x16 && format != rsx::texture::format::x32float)
			{
				auto remap_lambda = [](rsx::texture::component_remap op) {
					switch (op)
					{
					case rsx::texture::component_remap::A: return 0;
					case rsx::texture::component_remap::R: return 1;
					case rsx::texture::component_remap::G: return 2;
					case rsx::texture::component_remap::B: return 3;
					}
					throw;
				};

				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_A, glRemap[remap_lambda(tex.remap_0())]);
				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_R, glRemap[remap_lambda(tex.remap_1())]);
				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_G, glRemap[remap_lambda(tex.remap_2())]);
				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_B, glRemap[remap_lambda(tex.remap_3())]);
			}
			else
			{
				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_A, glRemap[0]);
				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_R, glRemap[1]);
				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_G, glRemap[2]);
				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_B, glRemap[3]);
			}

			__glcheck glTexParameteri(m_target, GL_TEXTURE_WRAP_S, gl_wrap(tex.wrap_s()));
			__glcheck glTexParameteri(m_target, GL_TEXTURE_WRAP_T, gl_wrap(tex.wrap_t()));
			__glcheck glTexParameteri(m_target, GL_TEXTURE_WRAP_R, gl_wrap(tex.wrap_r()));

			__glcheck glTexParameterf(m_target, GL_TEXTURE_LOD_BIAS, tex.bias());
			__glcheck glTexParameteri(m_target, GL_TEXTURE_MIN_LOD, (tex.min_lod() >> 8));
			__glcheck glTexParameteri(m_target, GL_TEXTURE_MAX_LOD, (tex.max_lod() >> 8));

			int min_filter = gl_tex_min_filter(tex.min_filter());
			
			if (min_filter != GL_LINEAR && min_filter != GL_NEAREST)
			{
				if (tex.get_exact_mipmap_count() <= 1 || m_target == GL_TEXTURE_RECTANGLE)
				{
					LOG_WARNING(RSX, "Texture %d, target 0x%X, requesting mipmap filtering without any mipmaps set!", m_id, m_target);
					min_filter = GL_LINEAR;
				}
			}

			__glcheck glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, min_filter);
			__glcheck glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, gl_tex_mag_filter(tex.mag_filter()));
			__glcheck glTexParameterf(m_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso(tex.max_aniso()));
		}

		void texture::init(int index, rsx::vertex_texture_t& tex)
		{
			switch (tex.dimension())
			{
			case rsx::texture::dimension::dimension3d:
				if (!tex.depth())
				{
					return;
				}

			case rsx::texture::dimension::dimension2d:
				if (!tex.height())
				{
					return;
				}

			case rsx::texture::dimension::dimension1d:
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

			rsx::texture::format format = tex.format();
			bool is_swizzled = tex.layout() == rsx::texture::layout::swizzled;

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

			/*
			if (format != CELL_GCM_TEXTURE_B8 && format != CELL_GCM_TEXTURE_X16 && format != CELL_GCM_TEXTURE_X32_FLOAT)
			{
				u8 remap_a = tex.remap() & 0x3;
				u8 remap_r = (tex.remap() >> 2) & 0x3;
				u8 remap_g = (tex.remap() >> 4) & 0x3;
				u8 remap_b = (tex.remap() >> 6) & 0x3;

				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_A, glRemap[remap_a]);
				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_R, glRemap[remap_r]);
				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_G, glRemap[remap_g]);
				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_B, glRemap[remap_b]);
			}
			else
			{
				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_A, glRemap[0]);
				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_R, glRemap[1]);
				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_G, glRemap[2]);
				__glcheck glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_B, glRemap[3]);
			}

			__glcheck glTexParameteri(m_target, GL_TEXTURE_WRAP_S, gl_wrap(tex.wrap_s()));
			__glcheck glTexParameteri(m_target, GL_TEXTURE_WRAP_T, gl_wrap(tex.wrap_t()));
			__glcheck glTexParameteri(m_target, GL_TEXTURE_WRAP_R, gl_wrap(tex.wrap_r()));
			*/

			__glcheck glTexParameterf(m_target, GL_TEXTURE_LOD_BIAS, tex.bias());
			__glcheck glTexParameteri(m_target, GL_TEXTURE_MIN_LOD, (tex.min_lod() >> 8));
			__glcheck glTexParameteri(m_target, GL_TEXTURE_MAX_LOD, (tex.max_lod() >> 8));

			int min_filter = gl_tex_min_filter(tex.min_filter());

			if (min_filter != GL_LINEAR && min_filter != GL_NEAREST)
			{
				if (tex.get_exact_mipmap_count() <= 1 || m_target == GL_TEXTURE_RECTANGLE)
				{
					LOG_WARNING(RSX, "Texture %d, target 0x%X, requesting mipmap filtering without any mipmaps set!", m_id, m_target);
					min_filter = GL_LINEAR;
				}
			}

			__glcheck glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, min_filter);
			__glcheck glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, gl_tex_mag_filter(tex.mag_filter()));
			__glcheck glTexParameterf(m_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso(tex.max_aniso()));
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
