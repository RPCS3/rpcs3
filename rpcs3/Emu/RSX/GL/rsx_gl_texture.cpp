#include "stdafx.h"
#include "rsx_gl_texture.h"
#include "gl_helpers.h"
#include "../GCM.h"
#include "../RSXThread.h"
#include "../RSXTexture.h"
#include "../rsx_utils.h"
#include "../Common/TextureUtils.h"

namespace rsx
{
	namespace gl
	{
		static const int gl_tex_min_filter[] =
		{
			GL_NEAREST, // unused
			GL_NEAREST,
			GL_LINEAR,
			GL_NEAREST_MIPMAP_NEAREST,
			GL_LINEAR_MIPMAP_NEAREST,
			GL_NEAREST_MIPMAP_LINEAR,
			GL_LINEAR_MIPMAP_LINEAR,
			GL_NEAREST, // CELL_GCM_TEXTURE_CONVOLUTION_MIN
		};

		static const int gl_tex_mag_filter[] =
		{
			GL_NEAREST, // unused
			GL_NEAREST,
			GL_LINEAR,
			GL_NEAREST, // unused
			GL_LINEAR  // CELL_GCM_TEXTURE_CONVOLUTION_MAG
		};

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

		int texture::gl_wrap(int wrap)
		{
			switch (wrap)
			{
			case CELL_GCM_TEXTURE_WRAP: return GL_REPEAT;
			case CELL_GCM_TEXTURE_MIRROR: return GL_MIRRORED_REPEAT;
			case CELL_GCM_TEXTURE_CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
			case CELL_GCM_TEXTURE_BORDER: return GL_CLAMP_TO_BORDER;
			case CELL_GCM_TEXTURE_CLAMP: return GL_CLAMP;
			case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP_TO_EDGE: return GL_MIRROR_CLAMP_TO_EDGE_EXT;
			case CELL_GCM_TEXTURE_MIRROR_ONCE_BORDER: return GL_MIRROR_CLAMP_TO_BORDER_EXT;
			case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP: return GL_MIRROR_CLAMP_EXT;
			}

			LOG_ERROR(RSX, "Texture wrap error: bad wrap (%d).", wrap);
			return GL_REPEAT;
		}

		float texture::max_aniso(int aniso)
		{
			switch (aniso)
			{
			case CELL_GCM_TEXTURE_MAX_ANISO_1: return 1.0f;
			case CELL_GCM_TEXTURE_MAX_ANISO_2: return 2.0f;
			case CELL_GCM_TEXTURE_MAX_ANISO_4: return 4.0f;
			case CELL_GCM_TEXTURE_MAX_ANISO_6: return 6.0f;
			case CELL_GCM_TEXTURE_MAX_ANISO_8: return 8.0f;
			case CELL_GCM_TEXTURE_MAX_ANISO_10: return 10.0f;
			case CELL_GCM_TEXTURE_MAX_ANISO_12: return 12.0f;
			case CELL_GCM_TEXTURE_MAX_ANISO_16: return 16.0f;
			}

			LOG_ERROR(RSX, "Texture anisotropy error: bad max aniso (%d).", aniso);
			return 1.0f;
		}

		u16 texture::get_pitch_modifier(u32 format)
		{
			switch (format)
			{
			case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
			case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
			default:
				LOG_ERROR(RSX, "Unimplemented Texture format : 0x%x", format);
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

		bool texture::mandates_expansion(u32 format)
		{
			/**
			 * If a texture behaves differently when uploaded directly vs when uploaded via texutils methods, it should be added here.
			 */
			if (format == CELL_GCM_TEXTURE_A1R5G5B5)
				return true;
			
			return false;
		}

		void texture::init(int index, rsx::texture& tex)
		{
			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());

			//TODO: safe init
			if (!m_id)
			{
				create();
			}

			glActiveTexture(GL_TEXTURE0 + index);
			bind();

			u32 full_format = tex.format();

			u32 format = full_format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
			bool is_swizzled = !!(~full_format & CELL_GCM_TEXTURE_LN);

			static const GLint glRemapStandard[4] = { GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE };
			// NOTE: This must be in ARGB order in all forms below.
			const GLint *glRemap = glRemapStandard;

			::gl::pixel_pack_settings().apply();
			::gl::pixel_unpack_settings().apply();

			u32 aligned_pitch = tex.pitch();

			size_t texture_data_sz = get_placed_texture_storage_size(tex, 256);
			std::vector<u8> data_upload_buf(texture_data_sz);
			u8* texture_data = data_upload_buf.data();
			u32 block_sz = get_pitch_modifier(format);

			if (is_swizzled || mandates_expansion(format))
			{
				aligned_pitch = align(aligned_pitch, 256);
				upload_placed_texture({ reinterpret_cast<gsl::byte*>(texture_data), gsl::narrow<int>(texture_data_sz) }, tex, 256);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			}
			else
			{
				texture_data = vm::ps3::_ptr<u8>(texaddr);
			}

			if (block_sz)
				aligned_pitch /= block_sz;
			else
				aligned_pitch = 0;

			glPixelStorei(GL_UNPACK_ROW_LENGTH, aligned_pitch);

			switch (format)
			{
			case CELL_GCM_TEXTURE_B8: // One 8-bit fixed-point number
			{
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BLUE, GL_UNSIGNED_BYTE, texture_data);

				static const GLint swizzleMaskB8[] = { GL_BLUE, GL_BLUE, GL_BLUE, GL_BLUE };
				glRemap = swizzleMaskB8;
				break;
			}

			case CELL_GCM_TEXTURE_A1R5G5B5:
			{
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, texture_data);
				break;
			}

			case CELL_GCM_TEXTURE_A4R4G4B4:
			{
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4, texture_data);

				// We read it in as R4G4B4A4, so we need to remap each component.
				static const GLint swizzleMaskA4R4G4B4[] = { GL_BLUE, GL_ALPHA, GL_RED, GL_GREEN };
				glRemap = swizzleMaskA4R4G4B4;
				break;
			}

			case CELL_GCM_TEXTURE_R5G6B5:
			{
				LOG_WARNING(RSX, "CELL_GCM_R5G6B5 texture. Watch out for corruption due to swapped color channels!");
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				glTexImage2D(m_target, 0, GL_RGB, tex.width(), tex.height(), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, texture_data);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
				break;
			}

			case CELL_GCM_TEXTURE_A8R8G8B8:
			{
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, texture_data);
				break;
			}

			case CELL_GCM_TEXTURE_COMPRESSED_DXT1: // Compressed 4x4 pixels into 8 bytes
			{
				u32 size = ((tex.width() + 3) / 4) * ((tex.height() + 3) / 4) * 8;
				glCompressedTexImage2D(m_target, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, tex.width(), tex.height(), 0, size, texture_data);
				break;
			}

			case CELL_GCM_TEXTURE_COMPRESSED_DXT23: // Compressed 4x4 pixels into 16 bytes
			{
				u32 size = ((tex.width() + 3) / 4) * ((tex.height() + 3) / 4) * 16;
				glCompressedTexImage2D(m_target, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, tex.width(), tex.height(), 0, size, texture_data);
			}
			break;

			case CELL_GCM_TEXTURE_COMPRESSED_DXT45: // Compressed 4x4 pixels into 16 bytes
			{
				u32 size = ((tex.width() + 3) / 4) * ((tex.height() + 3) / 4) * 16;
				glCompressedTexImage2D(m_target, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, tex.width(), tex.height(), 0, size, texture_data);
				break;
			}

			case CELL_GCM_TEXTURE_G8B8:
			{
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RG, GL_UNSIGNED_BYTE, texture_data);

				static const GLint swizzleMaskG8B8[] = { GL_RED, GL_GREEN, GL_RED, GL_GREEN };
				glRemap = swizzleMaskG8B8;
				break;
			}

			case CELL_GCM_TEXTURE_R6G5B5:
			{
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
				break;
			}

			case CELL_GCM_TEXTURE_DEPTH24_D8: //  24-bit unsigned fixed-point number and 8 bits of garbage
			{
				glTexImage2D(m_target, 0, GL_DEPTH_COMPONENT24, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, texture_data);
				break;
			}

			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: // 24-bit unsigned float and 8 bits of garbage
			{
				glTexImage2D(m_target, 0, GL_DEPTH_COMPONENT24, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, texture_data);
				break;
			}

			case CELL_GCM_TEXTURE_DEPTH16: // 16-bit unsigned fixed-point number
			{
				glTexImage2D(m_target, 0, GL_DEPTH_COMPONENT16, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_SHORT, texture_data);
				break;
			}

			case CELL_GCM_TEXTURE_DEPTH16_FLOAT: // 16-bit unsigned float
			{
				glTexImage2D(m_target, 0, GL_DEPTH_COMPONENT16, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, texture_data);
				break;
			}

			case CELL_GCM_TEXTURE_X16: // A 16-bit fixed-point number
			{
				LOG_WARNING(RSX, "CELL_GCM_X16 texture. Watch out for corruption due to swapped color channels!");
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RED, GL_UNSIGNED_SHORT, texture_data);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

				static const GLint swizzleMaskX16[] = { GL_RED, GL_ONE, GL_RED, GL_ONE };
				glRemap = swizzleMaskX16;
				break;
			}

			case CELL_GCM_TEXTURE_Y16_X16: // Two 16-bit fixed-point numbers
			{
				LOG_WARNING(RSX, "CELL_GCM_Y16_X16 texture. Watch out for corruption due to swapped color channels!");
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RG, GL_UNSIGNED_SHORT, texture_data);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

				static const GLint swizzleMaskX32_Y16_X16[] = { GL_GREEN, GL_RED, GL_GREEN, GL_RED };
				glRemap = swizzleMaskX32_Y16_X16;
				break;
			}

			case CELL_GCM_TEXTURE_R5G5B5A1:
			{
				LOG_WARNING(RSX, "CELL_GCM_R5G6B5A1 texture. Watch out for corruption due to swapped color channels!");
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, texture_data);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
				break;
			}

			case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: // Four fp16 values
			{
				LOG_WARNING(RSX, "CELL_GCM_W16_Z16_Y16_X16_FLOAT texture. Watch out for corruption due to swapped color channels!");
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_HALF_FLOAT, texture_data);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
				break;
			}

			case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: // Four fp32 values
			{
				LOG_WARNING(RSX, "CELL_GCM_W32_Z32_Y32_X32_FLOAT texture. Watch out for corruption due to swapped color channels!");
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_FLOAT, texture_data);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
				break;
			}

			case CELL_GCM_TEXTURE_X32_FLOAT: // One 32-bit floating-point number
			{
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RED, GL_FLOAT, texture_data);

				static const GLint swizzleMaskX32_FLOAT[] = { GL_RED, GL_ONE, GL_ONE, GL_ONE };
				glRemap = swizzleMaskX32_FLOAT;
				break;
			}

			case CELL_GCM_TEXTURE_D1R5G5B5:
			{
				LOG_WARNING(RSX, "CELL_GCM_D1R5G5B5 texture. Watch out for corruption due to swapped color channels!");
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);

				// TODO: Texture swizzling
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, texture_data);

				static const GLint swizzleMaskX32_D1R5G5B5[] = { GL_ONE, GL_RED, GL_GREEN, GL_BLUE };
				glRemap = swizzleMaskX32_D1R5G5B5;

				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
				break;
			}

			case CELL_GCM_TEXTURE_D8R8G8B8: // 8 bits of garbage and three unsigned 8-bit fixed-point numbers
			{
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, texture_data);

				static const GLint swizzleMaskX32_D8R8G8B8[] = { GL_ONE, GL_RED, GL_GREEN, GL_BLUE };
				glRemap = swizzleMaskX32_D8R8G8B8;
				break;
			}


			case CELL_GCM_TEXTURE_Y16_X16_FLOAT: // Two fp16 values
			{
				LOG_WARNING(RSX, "CELL_GCM_Y16_X16_FLOAT texture. Watch out for corruption due to swapped color channels!");
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RG, GL_HALF_FLOAT, texture_data);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

				static const GLint swizzleMaskX32_Y16_X16_FLOAT[] = { GL_RED, GL_GREEN, GL_RED, GL_GREEN };
				glRemap = swizzleMaskX32_Y16_X16_FLOAT;
				break;
			}

			case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
			case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			{
				glTexImage2D(m_target, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
				break;
			}

			default:
			{
				LOG_ERROR(RSX, "Init tex error: Bad tex format (0x%x | %s | 0x%x)", format, (is_swizzled ? "swizzled" : "linear"), tex.format() & 0x40);
				break;
			}
			}

			glTexParameteri(m_target, GL_TEXTURE_MAX_LEVEL, tex.mipmap() - 1);
			glTexParameteri(m_target, GL_GENERATE_MIPMAP, tex.mipmap() > 1);

			if (format != CELL_GCM_TEXTURE_B8 && format != CELL_GCM_TEXTURE_X16 && format != CELL_GCM_TEXTURE_X32_FLOAT)
			{
				u8 remap_a = tex.remap() & 0x3;
				u8 remap_r = (tex.remap() >> 2) & 0x3;
				u8 remap_g = (tex.remap() >> 4) & 0x3;
				u8 remap_b = (tex.remap() >> 6) & 0x3;

				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_A, glRemap[remap_a]);
				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_R, glRemap[remap_r]);
				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_G, glRemap[remap_g]);
				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_B, glRemap[remap_b]);
			}
			else
			{

				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_A, glRemap[0]);
				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_R, glRemap[1]);
				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_G, glRemap[2]);
				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_B, glRemap[3]);
			}

			glTexParameteri(m_target, GL_TEXTURE_WRAP_S, gl_wrap(tex.wrap_s()));
			glTexParameteri(m_target, GL_TEXTURE_WRAP_T, gl_wrap(tex.wrap_t()));
			glTexParameteri(m_target, GL_TEXTURE_WRAP_R, gl_wrap(tex.wrap_r()));

			glTexParameteri(m_target, GL_TEXTURE_COMPARE_FUNC, gl_tex_zfunc[tex.zfunc()]);

			glTexEnvi(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, (GLint)tex.bias());
			glTexParameteri(m_target, GL_TEXTURE_MIN_LOD, (tex.min_lod() >> 8));
			glTexParameteri(m_target, GL_TEXTURE_MAX_LOD, (tex.max_lod() >> 8));

			glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, gl_tex_min_filter[tex.min_filter()]);
			glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, gl_tex_mag_filter[tex.mag_filter()]);
			glTexParameterf(m_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso(tex.max_aniso()));
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
