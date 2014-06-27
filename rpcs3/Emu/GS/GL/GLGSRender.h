#pragma once
#include "Emu/GS/GSRender.h"
#include "Emu/GS/RSXThread.h"
#include "GLBuffers.h"
#include "GLProgramBuffer.h"

#pragma comment(lib, "opengl32.lib")

#define RSX_DEBUG 1

extern GLenum g_last_gl_error;
void printGlError(GLenum err, const char* situation);
void printGlError(GLenum err, const std::string& situation);
u32 LinearToSwizzleAddress(u32 x, u32 y, u32 z, u32 log2_width, u32 log2_height, u32 log2_depth);

#if RSX_DEBUG
#define checkForGlError(sit) if((g_last_gl_error = glGetError()) != GL_NO_ERROR) printGlError(g_last_gl_error, sit)
#else
#define checkForGlError(sit)
#endif

class GLTexture
{
	u32 m_id;
	
public:
	GLTexture() : m_id(0)
	{
	}

	void Create()
	{
		if(m_id)
		{
			Delete();
		}

		if(!m_id)
		{
			glGenTextures(1, &m_id);
			checkForGlError("GLTexture::Init() -> glGenTextures");
			Bind();
		}
	}

	int GetGlWrap(int wrap)
	{
		switch(wrap)
		{
		case 1: return GL_REPEAT;
		case 2: return GL_MIRRORED_REPEAT;
		case 3: return GL_CLAMP_TO_EDGE;
		case 4: return GL_CLAMP_TO_BORDER;
		case 5: return GL_CLAMP_TO_EDGE;
		case 6: return GL_MIRROR_CLAMP_TO_EDGE_EXT;
		case 7: return GL_MIRROR_CLAMP_TO_BORDER_EXT;
		case 8: return GL_MIRROR_CLAMP_EXT;
		}

		LOG_ERROR(RSX, "Texture wrap error: bad wrap (%d).", wrap);
		return GL_REPEAT;
	}

	inline static u8 Convert4To8(u8 v)
	{
		// Swizzle bits: 00001234 -> 12341234
		return (v << 4) | (v);
	}

	inline static u8 Convert5To8(u8 v)
	{
		// Swizzle bits: 00012345 -> 12345123
		return (v << 3) | (v >> 2);
	}

	inline static u8 Convert6To8(u8 v)
	{
		// Swizzle bits: 00123456 -> 12345612
		return (v << 2) | (v >> 4);
	}

	void Init(RSXTexture& tex)
	{
		if (tex.GetLocation() > 1)
			return;

		Bind();

		const u64 texaddr = GetAddress(tex.GetOffset(), tex.GetLocation());
		if (!Memory.IsGoodAddr(texaddr))
		{
			LOG_ERROR(RSX, "Bad texture address=0x%x", texaddr);
			return;
		}
		//ConLog.Warning("texture addr = 0x%x, width = %d, height = %d, max_aniso=%d, mipmap=%d, remap=0x%x, zfunc=0x%x, wraps=0x%x, wrapt=0x%x, wrapr=0x%x, minlod=0x%x, maxlod=0x%x", 
		//	m_offset, m_width, m_height, m_maxaniso, m_mipmap, m_remap, m_zfunc, m_wraps, m_wrapt, m_wrapr, m_minlod, m_maxlod);
		//TODO: safe init
		checkForGlError("GLTexture::Init() -> glBindTexture");

		int format = tex.GetFormat() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
		bool is_swizzled = !(tex.GetFormat() & CELL_GCM_TEXTURE_LN);

		const u8 *pixels = const_cast<const u8 *>(Memory.GetMemFromAddr(texaddr));
		u8 *unswizzledPixels;
		static const GLint glRemapStandard[4] = {GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE};
		// NOTE: This must be in ARGB order in all forms below.
		const GLint *glRemap = glRemapStandard;

		switch(format)
		{
		case CELL_GCM_TEXTURE_B8: // One 8-bit fixed-point number
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_BLUE, GL_UNSIGNED_BYTE, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_B8)");

			static const GLint swizzleMaskB8[] = { GL_BLUE, GL_BLUE, GL_BLUE, GL_BLUE };
			glRemap = swizzleMaskB8;
		}
		break;

		case CELL_GCM_TEXTURE_A1R5G5B5:
			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
			checkForGlError("GLTexture::Init() -> glPixelStorei");

			// TODO: texture swizzling
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_A1R5G5B5)");

			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
			checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_A1R5G5B5)");
		break;

		case CELL_GCM_TEXTURE_A4R4G4B4:
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_A4R4G4B4)");

			// We read it in as R4G4B4A4, so we need to remap each component.
			static const GLint swizzleMaskA4R4G4B4[] = { GL_BLUE, GL_ALPHA, GL_RED, GL_GREEN };
			glRemap = swizzleMaskA4R4G4B4;
		}
		break;

		case CELL_GCM_TEXTURE_R5G6B5:
		{
			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
			checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_R5G6B5)");

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex.GetWidth(), tex.GetHeight(), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_R5G6B5)");

			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
			checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_R5G6B5)");
		}
		break;
		
		case CELL_GCM_TEXTURE_A8R8G8B8:
			if(is_swizzled)
			{
				u32 *src, *dst;
				u32 log2width, log2height;

				unswizzledPixels = (u8*)malloc(tex.GetWidth() * tex.GetHeight() * 4);
				src = (u32*)pixels;
				dst = (u32*)unswizzledPixels;

				log2width = log(tex.GetWidth())/log(2);
				log2height = log(tex.GetHeight())/log(2);

				for(int i=0; i<tex.GetHeight(); i++)
				{
					for(int j=0; j<tex.GetWidth(); j++)
					{
						dst[(i*tex.GetHeight()) + j] = src[LinearToSwizzleAddress(j, i, 0, log2width, log2height, 0)];
					}
				}
			}

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, is_swizzled ? unswizzledPixels : pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_A8R8G8B8)");
		break;

		case CELL_GCM_TEXTURE_COMPRESSED_DXT1: // Compressed 4x4 pixels into 8 bytes
		{
			u32 size = ((tex.GetWidth() + 3) / 4) * ((tex.GetHeight() + 3) / 4) * 8;

			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, tex.GetWidth(), tex.GetHeight(), 0, size, pixels);
			checkForGlError("GLTexture::Init() -> glCompressedTexImage2D(CELL_GCM_TEXTURE_COMPRESSED_DXT1)");
		}
		break;

		case CELL_GCM_TEXTURE_COMPRESSED_DXT23: // Compressed 4x4 pixels into 16 bytes
		{
			u32 size = ((tex.GetWidth() + 3) / 4) * ((tex.GetHeight() + 3) / 4) * 16;

			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, tex.GetWidth(), tex.GetHeight(), 0, size, pixels);
			checkForGlError("GLTexture::Init() -> glCompressedTexImage2D(CELL_GCM_TEXTURE_COMPRESSED_DXT23)");
		}
		break;

		case CELL_GCM_TEXTURE_COMPRESSED_DXT45: // Compressed 4x4 pixels into 16 bytes
		{
			u32 size = ((tex.GetWidth() + 3) / 4) * ((tex.GetHeight() + 3) / 4) * 16;

			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, tex.GetWidth(), tex.GetHeight(), 0, size, pixels);
			checkForGlError("GLTexture::Init() -> glCompressedTexImage2D(CELL_GCM_TEXTURE_COMPRESSED_DXT45)");
		}
		break;

		case CELL_GCM_TEXTURE_G8B8:
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_RG, GL_UNSIGNED_BYTE, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_G8B8)");

			static const GLint swizzleMaskG8B8[] = { GL_RED, GL_GREEN, GL_RED, GL_GREEN };
			glRemap = swizzleMaskG8B8;
		}
		break;

		case CELL_GCM_TEXTURE_R6G5B5:
		{
			// TODO: Probably need to actually unswizzle if is_swizzled.
			const u32 numPixels = tex.GetWidth() * tex.GetHeight();
			unswizzledPixels = (u8 *)malloc(numPixels * 4);
			// TODO: Speed.
			for (u32 i = 0; i < numPixels; ++i) {
				u16 c = reinterpret_cast<const be_t<u16> *>(pixels)[i];
				unswizzledPixels[i * 4 + 0] = Convert6To8((c >> 10) & 0x3F);
				unswizzledPixels[i * 4 + 1] = Convert5To8((c >>  5) & 0x1F);
				unswizzledPixels[i * 4 + 2] = Convert5To8((c >>  0) & 0x1F);
				unswizzledPixels[i * 4 + 3] = 255;
			}

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, unswizzledPixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_R6G5B5)");

			free(unswizzledPixels);
		}
		break;

		case CELL_GCM_TEXTURE_DEPTH24_D8: //  24-bit unsigned fixed-point number and 8 bits of garbage
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, tex.GetWidth(), tex.GetHeight(), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_DEPTH24_D8)");
			break;

		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: // 24-bit unsigned float and 8 bits of garbage
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, tex.GetWidth(), tex.GetHeight(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT)");
			break;

		case CELL_GCM_TEXTURE_DEPTH16: // 16-bit unsigned fixed-point number
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, tex.GetWidth(), tex.GetHeight(), 0, GL_DEPTH_COMPONENT, GL_SHORT, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_DEPTH16)");
		break;

		case CELL_GCM_TEXTURE_DEPTH16_FLOAT: // 16-bit unsigned float
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, tex.GetWidth(), tex.GetHeight(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_DEPTH16_FLOAT)");
		break;
		
		case CELL_GCM_TEXTURE_X16: // A 16-bit fixed-point number
		{
			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
			checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_X16)");

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_RED, GL_UNSIGNED_SHORT, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_X16)");

			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
			checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_X16)");

			static const GLint swizzleMaskX16[] = { GL_RED, GL_ONE, GL_RED, GL_ONE };
			glRemap = swizzleMaskX16;
		}
		break;

		case CELL_GCM_TEXTURE_Y16_X16: // Two 16-bit fixed-point numbers
		{
			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
			checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_Y16_X16)");

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_RG, GL_UNSIGNED_SHORT, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_Y16_X16)");

			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
			checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_Y16_X16)");

			static const GLint swizzleMaskX32_Y16_X16[] = { GL_GREEN, GL_RED, GL_GREEN, GL_RED };
			glRemap = swizzleMaskX32_Y16_X16;
		}
		break;

		case CELL_GCM_TEXTURE_R5G5B5A1:
			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
			checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_R5G5B5A1)");

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_R5G5B5A1)");

			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
			checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_R5G5B5A1)");
		break;
		
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: // Four fp16 values
			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
			checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT)");

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_RGBA, GL_HALF_FLOAT, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT)");

			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
			checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT)");
		break;

		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: // Four fp32 values
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_BGRA, GL_FLOAT, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT)");
		break;

		case CELL_GCM_TEXTURE_X32_FLOAT: // One 32-bit floating-point number
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_RED, GL_FLOAT, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_X32_FLOAT)");

			static const GLint swizzleMaskX32_FLOAT[] = { GL_RED, GL_ONE, GL_ONE, GL_ONE };
			glRemap = swizzleMaskX32_FLOAT;
		}
		break;

		case CELL_GCM_TEXTURE_D1R5G5B5:
		{
			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
			checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_D1R5G5B5)");

			// TODO: Texture swizzling
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_D1R5G5B5)");

			static const GLint swizzleMaskX32_D1R5G5B5[] = { GL_ONE, GL_RED, GL_GREEN, GL_BLUE };
			glRemap = swizzleMaskX32_D1R5G5B5;

			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
			checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_D1R5G5B5)");
		}
		break;

		case CELL_GCM_TEXTURE_D8R8G8B8: // 8 bits of garbage and three unsigned 8-bit fixed-point numbers
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_D8R8G8B8)");

			static const GLint swizzleMaskX32_D8R8G8B8[] = { GL_ONE, GL_RED, GL_GREEN, GL_BLUE };
			glRemap = swizzleMaskX32_D8R8G8B8;
		}
		break;

		case CELL_GCM_TEXTURE_Y16_X16_FLOAT: // Two fp16 values
		{
			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
			checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_Y16_X16_FLOAT)");

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_RG, GL_HALF_FLOAT, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_Y16_X16_FLOAT)");

			glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
			checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_Y16_X16_FLOAT)");

			static const GLint swizzleMaskX32_Y16_X16_FLOAT[] = { GL_RED, GL_GREEN, GL_RED, GL_GREEN };
			glRemap = swizzleMaskX32_Y16_X16_FLOAT;
		}
		break;

		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8 & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN):
		{
			const u32 numPixels = tex.GetWidth() * tex.GetHeight();
			unswizzledPixels = (u8 *)malloc(numPixels * 4);
			// TODO: Speed.
			for (u32 i = 0; i < numPixels; i += 2) {
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

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, unswizzledPixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8 & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN)");

			free(unswizzledPixels);
		}
		break;

		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8 & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN):
		{
			const u32 numPixels = tex.GetWidth() * tex.GetHeight();
			unswizzledPixels = (u8 *)malloc(numPixels * 4);
			// TODO: Speed.
			for (u32 i = 0; i < numPixels; i += 2) {
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

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, unswizzledPixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8 & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN)");

			free(unswizzledPixels);
		}
		break;

		default: LOG_ERROR(RSX, "Init tex error: Bad tex format (0x%x | %s | 0x%x)", format,
					 (is_swizzled ? "swizzled" : "linear"), tex.GetFormat() & 0x40); break;
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, tex.GetMipmap() - 1);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, tex.GetMipmap() > 1);

		if (format != CELL_GCM_TEXTURE_B8 && format != CELL_GCM_TEXTURE_X16 && format != CELL_GCM_TEXTURE_X32_FLOAT)
		{
			u8 remap_a = tex.GetRemap() & 0x3;
			u8 remap_r = (tex.GetRemap() >> 2) & 0x3;
			u8 remap_g = (tex.GetRemap() >> 4) & 0x3;
			u8 remap_b = (tex.GetRemap() >> 6) & 0x3;

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
		
		checkForGlError("GLTexture::Init() -> remap");

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
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GetGlWrap(tex.GetWrapS()));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GetGlWrap(tex.GetWrapT()));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GetGlWrap(tex.GetWrapR()));

		checkForGlError("GLTexture::Init() -> wrap");

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, gl_tex_zfunc[tex.GetZfunc()]);

		checkForGlError("GLTexture::Init() -> compare");
		
		glTexEnvi(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, tex.GetBias());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, (tex.GetMinLOD() >> 8));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, (tex.GetMaxLOD() >> 8));

		checkForGlError("GLTexture::Init() -> lod");
		
		static const int gl_tex_filter[] =
		{
			GL_NEAREST,
			GL_NEAREST,
			GL_LINEAR,
			GL_NEAREST_MIPMAP_NEAREST,
			GL_LINEAR_MIPMAP_NEAREST,
			GL_NEAREST_MIPMAP_LINEAR,
			GL_LINEAR_MIPMAP_LINEAR,
			GL_NEAREST,
		};

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_tex_filter[tex.GetMinFilter()]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter[tex.GetMagFilter()]);

		checkForGlError("GLTexture::Init() -> filters");

		//Unbind();

		if(is_swizzled && format == CELL_GCM_TEXTURE_A8R8G8B8)
		{
			free(unswizzledPixels);
		}
	}

	void Save(RSXTexture& tex, const std::string& name)
	{
		if(!m_id || !tex.GetOffset() || !tex.GetWidth() || !tex.GetHeight()) return;

		const u32 texPixelCount = tex.GetWidth() * tex.GetHeight();

		u32* alldata = new u32[texPixelCount];

		Bind();

		switch(tex.GetFormat() & ~(0x20 | 0x40))
		{
		case CELL_GCM_TEXTURE_B8:
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, alldata);
		break;

		case CELL_GCM_TEXTURE_A8R8G8B8:
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, alldata);
		break;

		default:
			delete[] alldata;
		return;
		}

		{
			rFile f(name + ".raw", rFile::write);
			f.Write(alldata, texPixelCount * 4);
		}
		u8* data = new u8[texPixelCount * 3];
		u8* alpha = new u8[texPixelCount];

		u8* src = (u8*)alldata;
		u8* dst_d = data;
		u8* dst_a = alpha;
		for (u32 i = 0; i < texPixelCount; i++)
		{
			*dst_d++ = *src++;
			*dst_d++ = *src++;
			*dst_d++ = *src++;
			*dst_a++ = *src++;
		}

		rImage out;
		out.Create(tex.GetWidth(), tex.GetHeight(), data, alpha);
		out.SaveFile(name, rBITMAP_TYPE_PNG);

		delete[] alldata;
		//free(data);
		//free(alpha);
	}

	void Save(RSXTexture& tex)
	{
		static const std::string& dir_path = "textures";
		static const std::string& file_fmt = dir_path + "/" + "tex[%d].png";

		if(!rDirExists(dir_path)) rMkdir(dir_path);

		u32 count = 0;
		while(rFileExists(fmt::Format(file_fmt, count))) count++;
		Save(tex, fmt::Format(file_fmt, count));
	}

	void Bind()
	{
		glBindTexture(GL_TEXTURE_2D, m_id);
	}

	void Unbind()
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Delete()
	{
		if(m_id)
		{
			glDeleteTextures(1, &m_id);
			m_id = 0;
		}
	}
};

class PostDrawObj
{
protected:
	GLShaderProgram m_fp;
	GLVertexProgram m_vp;
	GLProgram m_program;
	GLfbo m_fbo;
	GLrbo m_rbo;

public:
	virtual void Draw()
	{
		static bool s_is_initialized = false;

		if(!s_is_initialized)
		{
			s_is_initialized = true;
			Initialize();
		}
		else
		{
			m_program.Use();
		}
	}

	virtual void InitializeShaders() = 0;
	virtual void InitializeLocations() = 0;

	void Initialize()
	{
		InitializeShaders();
		m_fp.Compile();
		m_vp.Compile();
		m_program.Create(m_vp.id, m_fp.GetId());
		m_program.Use();
		InitializeLocations();
	}
};

class DrawCursorObj : public PostDrawObj
{
	u32 m_tex_id;
	void* m_pixels;
	u32 m_width, m_height;
	double m_pos_x, m_pos_y, m_pos_z;
	bool m_update_texture, m_update_pos;

public:
	DrawCursorObj() : PostDrawObj()
		, m_tex_id(0)
		, m_update_texture(false)
		, m_update_pos(false)
	{
	}

	virtual void Draw()
	{
		checkForGlError("PostDrawObj : Unknown error.");

		PostDrawObj::Draw();
		checkForGlError("PostDrawObj::Draw");

		if(!m_fbo.IsCreated())
		{
			m_fbo.Create();
			checkForGlError("DrawCursorObj : m_fbo.Create");
			m_fbo.Bind();
			checkForGlError("DrawCursorObj : m_fbo.Bind");

			m_rbo.Create();
			checkForGlError("DrawCursorObj : m_rbo.Create");
			m_rbo.Bind();
			checkForGlError("DrawCursorObj : m_rbo.Bind");
			m_rbo.Storage(GL_RGBA, m_width, m_height);
			checkForGlError("DrawCursorObj : m_rbo.Storage");

			m_fbo.Renderbuffer(GL_COLOR_ATTACHMENT0, m_rbo.GetId());
			checkForGlError("DrawCursorObj : m_fbo.Renderbuffer");
		}

		m_fbo.Bind();
		checkForGlError("DrawCursorObj : m_fbo.Bind");
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		checkForGlError("DrawCursorObj : glDrawBuffer");

		m_program.Use();
		checkForGlError("DrawCursorObj : m_program.Use");

		if(m_update_texture)
		{
			//m_update_texture = false;

			glUniform2f(m_program.GetLocation("in_tc"), m_width, m_height);
			checkForGlError("DrawCursorObj : glUniform2f");
			if(!m_tex_id)
			{
				glGenTextures(1, &m_tex_id);
				checkForGlError("DrawCursorObj : glGenTextures");
			}

			glActiveTexture(GL_TEXTURE0);
			checkForGlError("DrawCursorObj : glActiveTexture");
			glBindTexture(GL_TEXTURE_2D, m_tex_id);
			checkForGlError("DrawCursorObj : glBindTexture");
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels);
			checkForGlError("DrawCursorObj : glTexImage2D");
			m_program.SetTex(0);
		}

		if(m_update_pos)
		{
			//m_update_pos = false;

			glUniform4f(m_program.GetLocation("in_pos"), m_pos_x, m_pos_y, m_pos_z, 1.0f);
			checkForGlError("DrawCursorObj : glUniform4f");
		}

		glDrawArrays(GL_QUADS, 0, 4);
		checkForGlError("DrawCursorObj : glDrawArrays");

		m_fbo.Bind(GL_READ_FRAMEBUFFER);
		checkForGlError("DrawCursorObj : m_fbo.Bind(GL_READ_FRAMEBUFFER)");
		GLfbo::Bind(GL_DRAW_FRAMEBUFFER, 0);
		checkForGlError("DrawCursorObj : GLfbo::Bind(GL_DRAW_FRAMEBUFFER, 0)");
		GLfbo::Blit(
			0, 0, m_width, m_height,
			0, 0, m_width, m_height,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
		checkForGlError("DrawCursorObj : GLfbo::Blit");
		m_fbo.Bind();
		checkForGlError("DrawCursorObj : m_fbo.Bind");
	}

	virtual void InitializeShaders()
	{
		m_vp.shader =
			"#version 330\n"
			"\n"
			"uniform vec4 in_pos;\n"
			"uniform vec2 in_tc;\n"
			"out vec2 tc;\n"
			"\n"
			"void main()\n"
			"{\n"
			"	tc = in_tc;\n"
			"	gl_Position = in_pos;\n"
			"}\n";

		m_fp.SetShaderText(
			"#version 330\n"
			"\n"
			"in vec2 tc;\n"
			"uniform sampler2D tex0;\n"
			"layout (location = 0) out vec4 res;\n"
			"\n"
			"void main()\n"
			"{\n"
			"	res = texture(tex0, tc);\n"
			"}\n");
	}

	void SetTexture(void* pixels, int width, int height)
	{
		m_pixels = pixels;
		m_width = width;
		m_height = height;

		m_update_texture = true;
	}

	void SetPosition(float x, float y, float z = 0.0f)
	{
		m_pos_x = x;
		m_pos_y = y;
		m_pos_z = z;
		m_update_pos = true;
	}

	void InitializeLocations()
	{
		//ConLog.Warning("tex0 location = 0x%x", m_program.GetLocation("tex0"));
	}
};

class GLGSRender //TODO: find out why this used to inherit from wxWindow
	: //public wxWindow
	/*,*/ public GSRender
{
private:
	std::vector<u8> m_vdata;
	std::vector<PostDrawObj> m_post_draw_objs;

	GLProgram m_program;
	int m_fp_buf_num;
	int m_vp_buf_num;
	GLProgramBuffer m_prog_buffer;

	GLShaderProgram m_shader_prog;
	GLVertexProgram m_vertex_prog;

	GLTexture m_gl_textures[m_textures_count];

	GLvao m_vao;
	GLvbo m_vbo;
	GLrbo m_rbo;
	GLfbo m_fbo;

	void* m_context;

public:
	rGLFrame* m_frame;
	u32 m_draw_frames;
	u32 m_skip_frames;

	GLGSRender();
	virtual ~GLGSRender();

private:
	void EnableVertexData(bool indexed_draw=false);
	void DisableVertexData();
	void InitVertexData();
	void InitFragmentData();

	void Enable(bool enable, const u32 cap);
	virtual void Close();
	bool LoadProgram();
	void WriteDepthBuffer();
	void WriteColorBuffers();
	void WriteColourBufferA();
	void WriteColourBufferB();
	void WriteColourBufferC();
	void WriteColourBufferD();

	void DrawObjects();

protected:
	virtual void OnInit();
	virtual void OnInitThread();
	virtual void OnExitThread();
	virtual void OnReset();
	virtual void ExecCMD();
	virtual void Flip();
};
