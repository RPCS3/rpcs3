#include "stdafx.h"
#include "rsx_gl_texture.h"
#include "gl_helpers.h"
#include "../GCM.h"
#include "../RSXThread.h"
#include "../RSXTexture.h"
#include "../rsx_utils.h"

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

		void texture::init(int index, rsx::texture& tex)
		{
			if (!m_id)
			{
				create();
			}

			glActiveTexture(GL_TEXTURE0 + index);
			bind();

			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
			//LOG_WARNING(RSX, "texture addr = 0x%x, width = %d, height = %d, max_aniso=%d, mipmap=%d, remap=0x%x, zfunc=0x%x, wraps=0x%x, wrapt=0x%x, wrapr=0x%x, minlod=0x%x, maxlod=0x%x", 
			//	m_offset, m_width, m_height, m_maxaniso, m_mipmap, m_remap, m_zfunc, m_wraps, m_wrapt, m_wrapr, m_minlod, m_maxlod);

			//TODO: safe init

			u32 full_format = tex.format();

			u32 format = full_format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
			bool is_swizzled = !!(~full_format & CELL_GCM_TEXTURE_LN);

			const u8* pixels = vm::ps3::_ptr<u8>(texaddr);
			static const GLint glRemapStandard[4] = { GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE };
			// NOTE: This must be in ARGB order in all forms below.
			const GLint *glRemap = glRemapStandard;

			::gl::pixel_pack_settings().apply();
			::gl::pixel_unpack_settings().apply();

			u32 pitch = tex.pitch();

			switch (format)
			{
			case CELL_GCM_TEXTURE_B8: // One 8-bit fixed-point number
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BLUE, GL_UNSIGNED_BYTE, pixels);

				static const GLint swizzleMaskB8[] = { GL_BLUE, GL_BLUE, GL_BLUE, GL_BLUE };
				glRemap = swizzleMaskB8;
				break;
			}

			case CELL_GCM_TEXTURE_A1R5G5B5:
			{
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 2);

				if (is_swizzled)
				{
					u16 height = tex.height();
					u16 width = tex.width();

					std::unique_ptr<u16[]> swz_buf(new u16[width * height]);
					u16 *src = (u16*)pixels;
					u16 *dst = swz_buf.get();

					rsx::convert_linear_swizzle<u16>(src, dst, width, height, true);

					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, dst);
					glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
					break;
				}
				
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, pixels);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
				break;
			}

			case CELL_GCM_TEXTURE_A4R4G4B4:
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 2);
				
				if (is_swizzled)
				{
					u16 height = tex.height();
					u16 width = tex.width();

					std::unique_ptr<u16[]> swz_buf(new u16[width * height]);
					u16 *src = (u16*)pixels;
					u16 *dst = swz_buf.get();

					rsx::convert_linear_swizzle<u16>(src, dst, width, height, true);

					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, dst);
				}
				else
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, pixels);

				// We read it in as R4G4B4A4, so we need to remap each component.
				static const GLint swizzleMaskA4R4G4B4[] = { GL_BLUE, GL_ALPHA, GL_RED, GL_GREEN };
				glRemap = swizzleMaskA4R4G4B4;
				break;
			}

			case CELL_GCM_TEXTURE_R5G6B5:
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 2);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex.width(), tex.height(), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
				break;
			}

			case CELL_GCM_TEXTURE_A8R8G8B8:
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 4);

				if (is_swizzled)
				{
					u16 height = tex.height();
					u16 width = tex.width();

					std::unique_ptr<u32[]> swz_buf(new u32[width * height]);
					u32 *src = (u32*)pixels;
					u32 *dst = swz_buf.get();

					if ((height & (height - 1)) || (width & (width - 1)))
					{
						LOG_ERROR(RSX, "Swizzle Texture: Width or height not power of 2! (h=%d,w=%d).", height, width);
					}

					rsx::convert_linear_swizzle<u32>(src, dst, width, height, true);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, dst);
				}
				else
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, pixels);
				
				break;
			}

			case CELL_GCM_TEXTURE_COMPRESSED_DXT1: // Compressed 4x4 pixels into 8 bytes
			{
				u32 size = ((tex.width() + 3) / 4) * ((tex.height() + 3) / 4) * 8;

				glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, tex.width(), tex.height(), 0, size, pixels);
				break;
			}

			case CELL_GCM_TEXTURE_COMPRESSED_DXT23: // Compressed 4x4 pixels into 16 bytes
			{
				u32 size = ((tex.width() + 3) / 4) * ((tex.height() + 3) / 4) * 16;

				glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, tex.width(), tex.height(), 0, size, pixels);
			}
			break;

			case CELL_GCM_TEXTURE_COMPRESSED_DXT45: // Compressed 4x4 pixels into 16 bytes
			{
				u32 size = ((tex.width() + 3) / 4) * ((tex.height() + 3) / 4) * 16;

				glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, tex.width(), tex.height(), 0, size, pixels);
				break;
			}

			case CELL_GCM_TEXTURE_G8B8:
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 2);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RG, GL_UNSIGNED_BYTE, pixels);

				static const GLint swizzleMaskG8B8[] = { GL_RED, GL_GREEN, GL_RED, GL_GREEN };
				glRemap = swizzleMaskG8B8;
				break;
			}

			case CELL_GCM_TEXTURE_R6G5B5:
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 2);

				// TODO: Probably need to actually unswizzle if is_swizzled.
				const u32 numPixels = tex.width() * tex.height();
				std::unique_ptr<u32[]> unpack_buf(new u32[numPixels]);
				u8 *unswizzledPixels = reinterpret_cast<u8*>(unpack_buf.get());
				// TODO: Speed.
				for (u32 i = 0; i < numPixels; ++i) {
					u16 c = reinterpret_cast<const be_t<u16> *>(pixels)[i];
					unswizzledPixels[i * 4 + 0] = convert_6_to_8((c >> 10) & 0x3F);
					unswizzledPixels[i * 4 + 1] = convert_5_to_8((c >> 5) & 0x1F);
					unswizzledPixels[i * 4 + 2] = convert_5_to_8((c >> 0) & 0x1F);
					unswizzledPixels[i * 4 + 3] = 255;
				}

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, unswizzledPixels);
				break;
			}

			case CELL_GCM_TEXTURE_DEPTH24_D8: //  24-bit unsigned fixed-point number and 8 bits of garbage
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 4);

				glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, pixels);
				break;
			}

			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: // 24-bit unsigned float and 8 bits of garbage
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 4);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, pixels);
				break;
			}

			case CELL_GCM_TEXTURE_DEPTH16: // 16-bit unsigned fixed-point number
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 2);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_SHORT, pixels);
				break;
			}

			case CELL_GCM_TEXTURE_DEPTH16_FLOAT: // 16-bit unsigned float
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 2);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, pixels);
				break;
			}

			case CELL_GCM_TEXTURE_X16: // A 16-bit fixed-point number
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 2);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RED, GL_UNSIGNED_SHORT, pixels);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

				static const GLint swizzleMaskX16[] = { GL_RED, GL_ONE, GL_RED, GL_ONE };
				glRemap = swizzleMaskX16;
				break;
			}

			case CELL_GCM_TEXTURE_Y16_X16: // Two 16-bit fixed-point numbers
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 4);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RG, GL_UNSIGNED_SHORT, pixels);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
				static const GLint swizzleMaskX32_Y16_X16[] = { GL_GREEN, GL_RED, GL_GREEN, GL_RED };
				glRemap = swizzleMaskX32_Y16_X16;
				break;
			}

			case CELL_GCM_TEXTURE_R5G5B5A1:
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 2);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, pixels);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
				break;
			}

			case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: // Four fp16 values
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 8);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_HALF_FLOAT, pixels);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
				break;
			}

			case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: // Four fp32 values
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 16);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_FLOAT, pixels);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
				break;
			}

			case CELL_GCM_TEXTURE_X32_FLOAT: // One 32-bit floating-point number
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 4);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RED, GL_FLOAT, pixels);

				static const GLint swizzleMaskX32_FLOAT[] = { GL_RED, GL_ONE, GL_ONE, GL_ONE };
				glRemap = swizzleMaskX32_FLOAT;
				break;
			}

			case CELL_GCM_TEXTURE_D1R5G5B5:
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 2);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);

				// TODO: Texture swizzling
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, pixels);

				static const GLint swizzleMaskX32_D1R5G5B5[] = { GL_ONE, GL_RED, GL_GREEN, GL_BLUE };
				glRemap = swizzleMaskX32_D1R5G5B5;

				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
				break;
			}

			case CELL_GCM_TEXTURE_D8R8G8B8: // 8 bits of garbage and three unsigned 8-bit fixed-point numbers
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 4);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, pixels);

				static const GLint swizzleMaskX32_D8R8G8B8[] = { GL_ONE, GL_RED, GL_GREEN, GL_BLUE };
				glRemap = swizzleMaskX32_D8R8G8B8;
				break;
			}


			case CELL_GCM_TEXTURE_Y16_X16_FLOAT: // Two fp16 values
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 4);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RG, GL_HALF_FLOAT, pixels);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

				static const GLint swizzleMaskX32_Y16_X16_FLOAT[] = { GL_RED, GL_GREEN, GL_RED, GL_GREEN };
				glRemap = swizzleMaskX32_Y16_X16_FLOAT;
				break;
			}

			case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 4);

				const u32 numPixels = tex.width() * tex.height();
				std::unique_ptr<u32[]> unpack_buf(new u32[numPixels]);
				u8 *unswizzledPixels = reinterpret_cast<u8*>(unpack_buf.get());
				// TODO: Speed.
				for (u32 i = 0; i < numPixels; i += 2)
				{
					unswizzledPixels[i * 4 + 0 + 0] = pixels[i * 2 + 3];
					unswizzledPixels[i * 4 + 0 + 1] = pixels[i * 2 + 2];
					unswizzledPixels[i * 4 + 0 + 2] = pixels[i * 2 + 0];
					unswizzledPixels[i * 4 + 0 + 3] = 255;

					// The second pixel is the same, except for red.
					unswizzledPixels[i * 4 + 4 + 0] = pixels[i * 2 + 1];
					unswizzledPixels[i * 4 + 4 + 1] = pixels[i * 2 + 2];
					unswizzledPixels[i * 4 + 4 + 2] = pixels[i * 2 + 0];
					unswizzledPixels[i * 4 + 4 + 3] = 255;
				}

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, unswizzledPixels);
				break;
			}

			case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 4);

				const u32 numPixels = tex.width() * tex.height();
				std::unique_ptr<u32[]> unpack_buf(new u32[numPixels]);
				u8 *unswizzledPixels = reinterpret_cast<u8*>(unpack_buf.get());
				// TODO: Speed.
				for (u32 i = 0; i < numPixels; i += 2)
				{
					unswizzledPixels[i * 4 + 0 + 0] = pixels[i * 2 + 2];
					unswizzledPixels[i * 4 + 0 + 1] = pixels[i * 2 + 3];
					unswizzledPixels[i * 4 + 0 + 2] = pixels[i * 2 + 1];
					unswizzledPixels[i * 4 + 0 + 3] = 255;

					// The second pixel is the same, except for red.
					unswizzledPixels[i * 4 + 4 + 0] = pixels[i * 2 + 0];
					unswizzledPixels[i * 4 + 4 + 1] = pixels[i * 2 + 3];
					unswizzledPixels[i * 4 + 4 + 2] = pixels[i * 2 + 1];
					unswizzledPixels[i * 4 + 4 + 3] = 255;
				}

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, unswizzledPixels);
				break;
			}

			default:
			{
				LOG_ERROR(RSX, "Init tex error: Bad tex format (0x%x | %s | 0x%x)", format, (is_swizzled ? "swizzled" : "linear"), tex.format() & 0x40);
				break;
			}
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, tex.mipmap() - 1);
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, tex.mipmap() > 1);

			if (format != CELL_GCM_TEXTURE_B8 && format != CELL_GCM_TEXTURE_X16 && format != CELL_GCM_TEXTURE_X32_FLOAT)
			{
				u8 remap_a = tex.remap() & 0x3;
				u8 remap_r = (tex.remap() >> 2) & 0x3;
				u8 remap_g = (tex.remap() >> 4) & 0x3;
				u8 remap_b = (tex.remap() >> 6) & 0x3;

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, glRemap[remap_a]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, glRemap[remap_r]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, glRemap[remap_g]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, glRemap[remap_b]);
			}
			else
			{

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, glRemap[0]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, glRemap[1]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, glRemap[2]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, glRemap[3]);
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_wrap(tex.wrap_s()));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_wrap(tex.wrap_t()));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, gl_wrap(tex.wrap_r()));

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, gl_tex_zfunc[tex.zfunc()]);

			glTexEnvi(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, (GLint)tex.bias());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, (tex.min_lod() >> 8));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, (tex.max_lod() >> 8));

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_tex_min_filter[tex.min_filter()]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_mag_filter[tex.mag_filter()]);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso(tex.max_aniso()));
		}

		void texture::bind()
		{
			glBindTexture(GL_TEXTURE_2D, m_id);
		}

		void texture::unbind()
		{
			glBindTexture(GL_TEXTURE_2D, 0);
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
