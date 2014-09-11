#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/rPlatform.h" // only for rImage
#include "Utilities/rFile.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "GLGSRender.h"

GetGSFrameCb GetGSFrame = nullptr;

void SetGetGSFrameCallback(GetGSFrameCb value)
{
	GetGSFrame = value;
}

#define CMD_DEBUG 0
#define DUMP_VERTEX_DATA 0

#if CMD_DEBUG
	#define CMD_LOG(...) LOG_NOTICE(RSX, __VA_ARGS__)
#else
	#define CMD_LOG(...)
#endif

GLuint g_flip_tex, g_depth_tex, g_pbo[6];
int last_width = 0, last_height = 0, last_depth_format = 0;

GLenum g_last_gl_error = GL_NO_ERROR;
void printGlError(GLenum err, const char* situation)
{
	if(err != GL_NO_ERROR)
	{
		LOG_ERROR(RSX, "%s: opengl error 0x%04x", situation, err);
		Emu.Pause();
	}
}
void printGlError(GLenum err, const std::string& situation)
{
	printGlError(err, situation.c_str());
}

#if 0
	#define checkForGlError(x) /*x*/
#endif

void GLTexture::Create()
{
	if (m_id)
	{
		Delete();
	}

	if (!m_id)
	{
		glGenTextures(1, &m_id);
		checkForGlError("GLTexture::Init() -> glGenTextures");
		Bind();
	}
}

int GLTexture::GetGlWrap(int wrap)
{
	switch (wrap)
	{
	case CELL_GCM_TEXTURE_WRAP: return GL_REPEAT;
	case CELL_GCM_TEXTURE_MIRROR: return GL_MIRRORED_REPEAT;
	case CELL_GCM_TEXTURE_CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
	case CELL_GCM_TEXTURE_BORDER: return GL_CLAMP_TO_BORDER;
	case CELL_GCM_TEXTURE_CLAMP: return GL_CLAMP_TO_EDGE;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP_TO_EDGE: return GL_MIRROR_CLAMP_TO_EDGE_EXT;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_BORDER: return GL_MIRROR_CLAMP_TO_BORDER_EXT;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP: return GL_MIRROR_CLAMP_EXT;
	}

	LOG_ERROR(RSX, "Texture wrap error: bad wrap (%d).", wrap);
	return GL_REPEAT;
}

float GLTexture::GetMaxAniso(int aniso)
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

void GLTexture::Init(RSXTexture& tex)
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

	auto pixels = vm::get_ptr<const u8>(texaddr);
	u8 *unswizzledPixels;
	static const GLint glRemapStandard[4] = { GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE };
	// NOTE: This must be in ARGB order in all forms below.
	const GLint *glRemap = glRemapStandard;

	switch (format)
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
		if (is_swizzled)
		{
			u32 *src, *dst;
			u32 log2width, log2height;

			unswizzledPixels = (u8*)malloc(tex.GetWidth() * tex.GetHeight() * 4);
			src = (u32*)pixels;
			dst = (u32*)unswizzledPixels;

			log2width = log(tex.GetWidth()) / log(2);
			log2height = log(tex.GetHeight()) / log(2);

			for (int i = 0; i<tex.GetHeight(); i++)
			{
				for (int j = 0; j<tex.GetWidth(); j++)
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
			unswizzledPixels[i * 4 + 1] = Convert5To8((c >> 5) & 0x1F);
			unswizzledPixels[i * 4 + 2] = Convert5To8((c >> 0) & 0x1F);
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

	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8 & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) :
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

	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8 & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) :
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

	static const int gl_tex_mag_filter[] = {
		GL_NEAREST, // unused
		GL_NEAREST,
		GL_LINEAR,
		GL_NEAREST, // unused
		GL_LINEAR  // CELL_GCM_TEXTURE_CONVOLUTION_MAG
	};

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_tex_min_filter[tex.GetMinFilter()]);

	checkForGlError("GLTexture::Init() -> min filters");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_mag_filter[tex.GetMagFilter()]);

	checkForGlError("GLTexture::Init() -> mag filters");
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, GetMaxAniso(tex.GetMaxAniso()));

	checkForGlError("GLTexture::Init() -> max anisotropy");

	//Unbind();

	if (is_swizzled && format == CELL_GCM_TEXTURE_A8R8G8B8)
	{
		free(unswizzledPixels);
	}
}

void GLTexture::Save(RSXTexture& tex, const std::string& name)
{
	if (!m_id || !tex.GetOffset() || !tex.GetWidth() || !tex.GetHeight()) return;

	const u32 texPixelCount = tex.GetWidth() * tex.GetHeight();

	u32* alldata = new u32[texPixelCount];

	Bind();

	switch (tex.GetFormat() & ~(0x20 | 0x40))
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

void GLTexture::Save(RSXTexture& tex)
{
	static const std::string& dir_path = "textures";
	static const std::string& file_fmt = dir_path + "/" + "tex[%d].png";

	if (!rExists(dir_path)) rMkdir(dir_path);

	u32 count = 0;
	while (rExists(fmt::Format(file_fmt, count))) count++;
	Save(tex, fmt::Format(file_fmt, count));
}

void GLTexture::Bind()
{
	glBindTexture(GL_TEXTURE_2D, m_id);
}

void GLTexture::Unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GLTexture::Delete()
{
	if (m_id)
	{
		glDeleteTextures(1, &m_id);
		m_id = 0;
	}
}

void PostDrawObj::Draw()
{
	static bool s_is_initialized = false;

	if (!s_is_initialized)
	{
		s_is_initialized = true;
		Initialize();
	}
	else
	{
		m_program.Use();
	}
}

void PostDrawObj::Initialize()
{
	InitializeShaders();
	m_fp.Compile();
	m_vp.Compile();
	m_program.Create(m_vp.id, m_fp.GetId());
	m_program.Use();
	InitializeLocations();
}

void DrawCursorObj::Draw()
{
	checkForGlError("PostDrawObj : Unknown error.");

	PostDrawObj::Draw();
	checkForGlError("PostDrawObj::Draw");

	if (!m_fbo.IsCreated())
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

	if (m_update_texture)
	{
		//m_update_texture = false;

		glUniform2f(m_program.GetLocation("in_tc"), m_width, m_height);
		checkForGlError("DrawCursorObj : glUniform2f");
		if (!m_tex_id)
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

	if (m_update_pos)
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

void DrawCursorObj::InitializeShaders()
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

void DrawCursorObj::SetTexture(void* pixels, int width, int height)
{
	m_pixels = pixels;
	m_width = width;
	m_height = height;

	m_update_texture = true;
}

void DrawCursorObj::SetPosition(float x, float y, float z)
{
	m_pos_x = x;
	m_pos_y = y;
	m_pos_z = z;
	m_update_pos = true;
}

void DrawCursorObj::InitializeLocations()
{
	//ConLog.Warning("tex0 location = 0x%x", m_program.GetLocation("tex0"));
}

GLGSRender::GLGSRender()
	: GSRender()
	, m_frame(nullptr)
	, m_fp_buf_num(-1)
	, m_vp_buf_num(-1)
	, m_context(nullptr)
{
	m_frame = GetGSFrame();
}

GLGSRender::~GLGSRender()
{
	m_frame->Close();
	m_frame->DeleteContext(m_context);
}

void GLGSRender::Enable(bool enable, const u32 cap)
{
	if(enable)
	{
		glEnable(cap);
	}
	else
	{
		glDisable(cap);
	}
}

extern CellGcmContextData current_context;

void GLGSRender::Close()
{
	Stop();

	if(m_frame->IsShown()) m_frame->Hide();
	m_ctrl = nullptr;
}

void GLGSRender::EnableVertexData(bool indexed_draw)
{
	static u32 offset_list[m_vertex_count];
	u32 cur_offset = 0;

	const u32 data_offset = indexed_draw ? 0 : m_draw_array_first;

	for(u32 i=0; i<m_vertex_count; ++i)
	{
		offset_list[i] = cur_offset;

		if(!m_vertex_data[i].IsEnabled() || !m_vertex_data[i].addr) continue;

		const size_t item_size = m_vertex_data[i].GetTypeSize() * m_vertex_data[i].size;
		const size_t data_size = m_vertex_data[i].data.size() - data_offset * item_size;
		const u32 pos = m_vdata.size();

		cur_offset += data_size;
		m_vdata.resize(m_vdata.size() + data_size);
		memcpy(&m_vdata[pos], &m_vertex_data[i].data[data_offset * item_size], data_size);
	}

	m_vao.Create();
	m_vao.Bind();
	checkForGlError("initializing vao");

	m_vbo.Create(indexed_draw ? 2 : 1);
	m_vbo.Bind(0);
	m_vbo.SetData(&m_vdata[0], m_vdata.size());

	if(indexed_draw)
	{
		m_vbo.Bind(GL_ELEMENT_ARRAY_BUFFER, 1);
		m_vbo.SetData(GL_ELEMENT_ARRAY_BUFFER, &m_indexed_array.m_data[0], m_indexed_array.m_data.size());
	}

	checkForGlError("initializing vbo");

#if	DUMP_VERTEX_DATA
	rFile dump("VertexDataArray.dump", rFile::write);
#endif

	for(u32 i=0; i<m_vertex_count; ++i)
	{
		if(!m_vertex_data[i].IsEnabled()) continue;

#if	DUMP_VERTEX_DATA
		dump.Write(wxString::Format("VertexData[%d]:\n", i));
		switch(m_vertex_data[i].type)
		{
		case CELL_GCM_VERTEX_S1:
			for(u32 j = 0; j<m_vertex_data[i].data.size(); j+=2)
			{
				dump.Write(wxString::Format("%d\n", *(u16*)&m_vertex_data[i].data[j]));
				if(!(((j+2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		case CELL_GCM_VERTEX_F:
			for(u32 j = 0; j<m_vertex_data[i].data.size(); j+=4)
			{
				dump.Write(wxString::Format("%.01f\n", *(float*)&m_vertex_data[i].data[j]));
				if(!(((j+4) / 4) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		case CELL_GCM_VERTEX_SF:
			for(u32 j = 0; j<m_vertex_data[i].data.size(); j+=2)
			{
				dump.Write(wxString::Format("%.01f\n", *(float*)&m_vertex_data[i].data[j]));
				if(!(((j+2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		case CELL_GCM_VERTEX_UB:
			for(u32 j = 0; j<m_vertex_data[i].data.size(); ++j)
			{
				dump.Write(wxString::Format("%d\n", m_vertex_data[i].data[j]));
				if(!((j+1) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		case CELL_GCM_VERTEX_S32K:
			for(u32 j = 0; j<m_vertex_data[i].data.size(); j+=2)
			{
				dump.Write(wxString::Format("%d\n", *(u16*)&m_vertex_data[i].data[j]));
				if(!(((j+2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;
		
		// case CELL_GCM_VERTEX_CMP:
		
		case CELL_GCM_VERTEX_UB256:
			for(u32 j = 0; j<m_vertex_data[i].data.size(); ++j)
			{
				dump.Write(wxString::Format("%d\n", m_vertex_data[i].data[j]));
				if(!((j+1) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		default:
			LOG_ERROR(HLE, "Bad cv type! %d", m_vertex_data[i].type);
		return;
		}

		dump.Write("\n");
#endif

		static const u32 gl_types[] =
		{
			GL_SHORT,
			GL_FLOAT,
			GL_HALF_FLOAT,
			GL_UNSIGNED_BYTE,
			GL_SHORT,
			GL_FLOAT, // Needs conversion
			GL_UNSIGNED_BYTE,
		};

		static const bool gl_normalized[] =
		{
			GL_TRUE,
			GL_FALSE,
			GL_FALSE,
			GL_TRUE,
			GL_FALSE,
			GL_TRUE,
			GL_FALSE,
		};

		if (m_vertex_data[i].type < 1 || m_vertex_data[i].type > 7) {
			LOG_ERROR(RSX, "GLGSRender::EnableVertexData: Bad vertex data type (%d)!", m_vertex_data[i].type);
		}

		if(!m_vertex_data[i].addr)
		{
			switch(m_vertex_data[i].type)
			{
			case CELL_GCM_VERTEX_S32K:
			case CELL_GCM_VERTEX_S1:
				switch(m_vertex_data[i].size)
				{
				case 1: glVertexAttrib1s(i, (GLshort&)m_vertex_data[i].data[0]); break;
				case 2: glVertexAttrib2sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
				case 3: glVertexAttrib3sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
				case 4: glVertexAttrib4sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
				}
			break;

			case CELL_GCM_VERTEX_F:
				switch(m_vertex_data[i].size)
				{
				case 1: glVertexAttrib1f(i, (GLfloat&)m_vertex_data[i].data[0]); break;
				case 2: glVertexAttrib2fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
				case 3: glVertexAttrib3fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
				case 4: glVertexAttrib4fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
				}
			break;

			case CELL_GCM_VERTEX_CMP:
			case CELL_GCM_VERTEX_UB:
				glVertexAttrib4ubv(i, (GLubyte*)&m_vertex_data[i].data[0]);
			break;
			}

			checkForGlError("glVertexAttrib");
		}
		else
		{
			u32 gltype = gl_types[m_vertex_data[i].type - 1];
			bool normalized = gl_normalized[m_vertex_data[i].type - 1];

			glEnableVertexAttribArray(i);
			checkForGlError("glEnableVertexAttribArray");
			glVertexAttribPointer(i, m_vertex_data[i].size, gltype, normalized, 0, reinterpret_cast<void*>(offset_list[i]));
			checkForGlError("glVertexAttribPointer");
		}
	}
}

void GLGSRender::DisableVertexData()
{
	m_vdata.clear();
	for(u32 i=0; i<m_vertex_count; ++i)
	{
		if(!m_vertex_data[i].IsEnabled() || !m_vertex_data[i].addr) continue;
		glDisableVertexAttribArray(i);
		checkForGlError("glDisableVertexAttribArray");
	}
	m_vao.Unbind();
}

void GLGSRender::InitVertexData()
{
	int l;
	GLfloat scaleOffsetMat[16] = {
		1.0f, 0.0f, 0.0f, 0.0f, 
		0.0f, 1.0f, 0.0f, 0.0f, 
		0.0f, 0.0f, 1.0f, 0.0f, 
		0.0f, 0.0f, 0.0f, 1.0f
	};

	for (const RSXTransformConstant& c : m_transform_constants) {
		const std::string name = fmt::Format("vc[%u]", c.id);
		l = m_program.GetLocation(name);
		checkForGlError("glGetUniformLocation " + name);

		//ConLog.Write(name + " x: %.02f y: %.02f z: %.02f w: %.02f", c.x, c.y, c.z, c.w);
		glUniform4f(l, c.x, c.y, c.z, c.w);
		checkForGlError("glUniform4f " + name + fmt::Format(" %d [%f %f %f %f]", l, c.x, c.y, c.z, c.w));
	}

	// Scale
	scaleOffsetMat[0]  = (GLfloat&)methodRegisters[NV4097_SET_VIEWPORT_SCALE + (0x4 * 0)] / (RSXThread::m_width / RSXThread::m_width_scale);
	scaleOffsetMat[5]  = (GLfloat&)methodRegisters[NV4097_SET_VIEWPORT_SCALE + (0x4 * 1)] / (RSXThread::m_height / RSXThread::m_height_scale);
	scaleOffsetMat[10] = (GLfloat&)methodRegisters[NV4097_SET_VIEWPORT_SCALE + (0x4 * 2)];

	// Offset
	scaleOffsetMat[3]  = (GLfloat&)methodRegisters[NV4097_SET_VIEWPORT_OFFSET + (0x4 * 0)] - (RSXThread::m_width / RSXThread::m_width_scale);
	scaleOffsetMat[7]  = (GLfloat&)methodRegisters[NV4097_SET_VIEWPORT_OFFSET + (0x4 * 1)] - (RSXThread::m_height / RSXThread::m_height_scale);
	scaleOffsetMat[11] = (GLfloat&)methodRegisters[NV4097_SET_VIEWPORT_OFFSET + (0x4 * 2)] - 1 / 2.0f;

	scaleOffsetMat[3] /= RSXThread::m_width / RSXThread::m_width_scale;
	scaleOffsetMat[7] /= RSXThread::m_height / RSXThread::m_height_scale;

	l = m_program.GetLocation("scaleOffsetMat");
	glUniformMatrix4fv(l, 1, false, scaleOffsetMat);
	checkForGlError("glUniformMatrix4fv");
}

void GLGSRender::InitFragmentData()
{
	if(!m_cur_shader_prog)
	{
		LOG_ERROR(RSX, "InitFragmentData: m_cur_shader_prog == NULL");
		return;
	}

	for(const RSXTransformConstant& c : m_fragment_constants) {
		u32 id = c.id - m_cur_shader_prog->offset;

		//LOG_WARNING(RSX,"fc%u[0x%x - 0x%x] = (%f, %f, %f, %f)", id, c.id, m_cur_shader_prog->offset, c.x, c.y, c.z, c.w);

		const std::string name = fmt::Format("fc%u", id);
		const int l = m_program.GetLocation(name);
		checkForGlError("glGetUniformLocation " + name);

		glUniform4f(l, c.x, c.y, c.z, c.w);
		checkForGlError("glUniform4f " + name + fmt::Format(" %u [%f %f %f %f]", l, c.x, c.y, c.z, c.w));
	}

	//if(m_fragment_constants.GetCount())
	//	LOG_NOTICE(HLE, "");
}

bool GLGSRender::LoadProgram()
{
	if(!m_cur_shader_prog)
	{
		LOG_WARNING(RSX, "LoadProgram: m_cur_shader_prog == NULL");
		return false;
	}
	
	m_cur_shader_prog->ctrl = m_shader_ctrl;
	
	if(!m_cur_vertex_prog)
	{
		LOG_WARNING(RSX, "LoadProgram: m_cur_vertex_prog == NULL");
		return false;
	}
	
	m_fp_buf_num = m_prog_buffer.SearchFp(*m_cur_shader_prog, m_shader_prog);
	m_vp_buf_num = m_prog_buffer.SearchVp(*m_cur_vertex_prog, m_vertex_prog);

	//ConLog.Write("Create program");

	if(m_fp_buf_num == -1)
	{
		LOG_WARNING(RSX, "FP not found in buffer!");
		//m_shader_prog.DecompileAsync(*m_cur_shader_prog);
		//m_shader_prog.Wait();
		m_shader_prog.Decompile(*m_cur_shader_prog);
		m_shader_prog.Compile();
		checkForGlError("m_shader_prog.Compile");

		// TODO: This shouldn't use current dir
		rFile f("./FragmentProgram.txt", rFile::write);
		f.Write(m_shader_prog.GetShaderText());
	}

	if(m_vp_buf_num == -1)
	{
		LOG_WARNING(RSX, "VP not found in buffer!");
		//m_vertex_prog.DecompileAsync(*m_cur_vertex_prog);
		//m_vertex_prog.Wait();
		m_vertex_prog.Decompile(*m_cur_vertex_prog);
		m_vertex_prog.Compile();
		checkForGlError("m_vertex_prog.Compile");

		// TODO: This shouldn't use current dir
		rFile f("./VertexProgram.txt", rFile::write);
		f.Write(m_vertex_prog.shader);
	}

	if(m_fp_buf_num != -1 && m_vp_buf_num != -1)
	{
		m_program.id = m_prog_buffer.GetProg(m_fp_buf_num, m_vp_buf_num);
	}

	if(m_program.id)
	{
		// RSX Debugger: Check if this program was modified and update it
		if (Ini.GSLogPrograms.GetValue())
		{
			for(auto& program : m_debug_programs)
			{
				if (program.id == m_program.id && program.modified)
				{
					// TODO: This isn't working perfectly. Is there any better/shorter way to update the program
					m_vertex_prog.shader = program.vp_shader;
					m_shader_prog.SetShaderText(program.fp_shader);
					m_vertex_prog.Wait();
					m_vertex_prog.Compile();
					checkForGlError("m_vertex_prog.Compile");
					m_shader_prog.Wait();
					m_shader_prog.Compile();
					checkForGlError("m_shader_prog.Compile");
					glAttachShader(m_program.id, m_vertex_prog.id);
					glAttachShader(m_program.id, m_shader_prog.GetId());
					glLinkProgram(m_program.id);
					checkForGlError("glLinkProgram");
					glDetachShader(m_program.id, m_vertex_prog.id);
					glDetachShader(m_program.id, m_shader_prog.GetId());
					program.vp_id = m_vertex_prog.id;
					program.fp_id = m_shader_prog.GetId();
					program.modified = false;
				}
			}
		}
		m_program.Use();
	}
	else
	{
		m_program.Create(m_vertex_prog.id, m_shader_prog.GetId());
		checkForGlError("m_program.Create");
		m_prog_buffer.Add(m_program, m_shader_prog, *m_cur_shader_prog, m_vertex_prog, *m_cur_vertex_prog);
		checkForGlError("m_prog_buffer.Add");
		m_program.Use();

		// RSX Debugger
		if (Ini.GSLogPrograms.GetValue())
		{
			RSXDebuggerProgram program;
			program.id = m_program.id;
			program.vp_id = m_vertex_prog.id;
			program.fp_id = m_shader_prog.GetId();
			program.vp_shader = m_vertex_prog.shader;
			program.fp_shader = m_shader_prog.GetShaderText();
			m_debug_programs.push_back(program);
		}
	}

	return true;
}

void GLGSRender::WriteBuffers()
{
	if (Ini.GSDumpDepthBuffer.GetValue())
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[4]);
		glBufferData(GL_PIXEL_PACK_BUFFER, RSXThread::m_width * RSXThread::m_height * 4, 0, GL_STREAM_READ);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		WriteDepthBuffer();
	}

	if (Ini.GSDumpColorBuffers.GetValue())
	{
		WriteColorBuffers();
	}
}

void GLGSRender::WriteDepthBuffer()
{
	if (!m_set_context_dma_z)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_z, m_context_dma_z - 0xfeed0000);
	if (!Memory.IsGoodAddr(address))
	{
		LOG_WARNING(RSX, "Bad depth buffer address: address=0x%x, offset=0x%x, dma=0x%x", address, m_surface_offset_z, m_context_dma_z);
		return;
	}

	auto ptr = vm::get_ptr<void>(address);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[4]);
	checkForGlError("WriteDepthBuffer(): glBindBuffer");
	glReadPixels(0, 0, RSXThread::m_width, RSXThread::m_height, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	checkForGlError("WriteDepthBuffer(): glReadPixels");
	GLubyte *packed = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if (packed)
	{
		memcpy(ptr, packed, RSXThread::m_width * RSXThread::m_height * 4);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		checkForGlError("WriteDepthBuffer(): glUnmapBuffer");
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	checkForGlError("WriteDepthBuffer(): glReadPixels");
	glBindTexture(GL_TEXTURE_2D, g_depth_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RSXThread::m_width, RSXThread::m_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, ptr);
	checkForGlError("WriteDepthBuffer(): glTexImage2D");
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, ptr);
	checkForGlError("WriteDepthBuffer(): glGetTexImage");
}

void GLGSRender::WriteColorBufferA()
{
	if (!m_set_context_dma_color_a)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_a, m_context_dma_color_a - 0xfeed0000);
	if (!Memory.IsGoodAddr(address))
	{
		LOG_ERROR(RSX, "Bad color buffer A address: address=0x%x, offset=0x%x, dma=0x%x", address, m_surface_offset_a, m_context_dma_color_a);
		return;
	}

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	checkForGlError("WriteColorBufferA(): glReadBuffer");
	glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[0]);
	checkForGlError("WriteColorBufferA(): glBindBuffer");
	glReadPixels(0, 0, RSXThread::m_width, RSXThread::m_height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, 0);
	checkForGlError("WriteColorBufferA(): glReadPixels");
	GLubyte *packed = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if (packed)
	{
		memcpy(vm::get_ptr<void>(address), packed, RSXThread::m_width * RSXThread::m_height * 4);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		checkForGlError("WriteColorBufferA(): glUnmapBuffer");
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void GLGSRender::WriteColorBufferB()
{
	if (!m_set_context_dma_color_b)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_b, m_context_dma_color_b - 0xfeed0000);
	if (!Memory.IsGoodAddr(address))
	{
		LOG_ERROR(RSX, "Bad color buffer B address: address=0x%x, offset=0x%x, dma=0x%x", address, m_surface_offset_b, m_context_dma_color_b);
		return;
	}

	glReadBuffer(GL_COLOR_ATTACHMENT1);
	checkForGlError("WriteColorBufferB(): glReadBuffer");
	glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[1]);
	checkForGlError("WriteColorBufferB(): glBindBuffer");
	glReadPixels(0, 0, RSXThread::m_width, RSXThread::m_height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, 0);
	checkForGlError("WriteColorBufferB(): glReadPixels");
	GLubyte *packed = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if (packed)
	{
		memcpy(vm::get_ptr<void>(address), packed, RSXThread::m_width * RSXThread::m_height * 4);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		checkForGlError("WriteColorBufferB(): glUnmapBuffer");
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void GLGSRender::WriteColorBufferC()
{
	if (!m_set_context_dma_color_c)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_c, m_context_dma_color_c - 0xfeed0000);
	if (!Memory.IsGoodAddr(address))
	{
		LOG_ERROR(RSX, "Bad color buffer C address: address=0x%x, offset=0x%x, dma=0x%x", address, m_surface_offset_c, m_context_dma_color_c);
		return;
	}

	glReadBuffer(GL_COLOR_ATTACHMENT2);
	checkForGlError("WriteColorBufferC(): glReadBuffer");
	glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[2]);
	checkForGlError("WriteColorBufferC(): glBindBuffer");
	glReadPixels(0, 0, RSXThread::m_width, RSXThread::m_height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, 0);
	checkForGlError("WriteColorBufferC(): glReadPixels");
	GLubyte *packed = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if (packed)
	{
		memcpy(vm::get_ptr<void>(address), packed, RSXThread::m_width * RSXThread::m_height * 4);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		checkForGlError("WriteColorBufferC(): glUnmapBuffer");
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void GLGSRender::WriteColorBufferD()
{
	if (!m_set_context_dma_color_d)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_d, m_context_dma_color_d - 0xfeed0000);
	if (!Memory.IsGoodAddr(address))
	{
		LOG_ERROR(RSX, "Bad color buffer D address: address=0x%x, offset=0x%x, dma=0x%x", address, m_surface_offset_d, m_context_dma_color_d);
		return;
	}

	glReadBuffer(GL_COLOR_ATTACHMENT3);
	checkForGlError("WriteColorBufferD(): glReadBuffer");
	glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[3]);
	checkForGlError("WriteColorBufferD(): glBindBuffer");
	glReadPixels(0, 0, RSXThread::m_width, RSXThread::m_height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, 0);
	checkForGlError("WriteColorBufferD(): glReadPixels");
	GLubyte *packed = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if (packed)
	{
		memcpy(vm::get_ptr<void>(address), packed, RSXThread::m_width * RSXThread::m_height * 4);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		checkForGlError("WriteColorBufferD(): glUnmapBuffer");
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

}

void GLGSRender::WriteColorBuffers()
{
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	switch(m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_NONE:
		return;

	case CELL_GCM_SURFACE_TARGET_0:
		glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[0]);
		glBufferData(GL_PIXEL_PACK_BUFFER, RSXThread::m_width * RSXThread::m_height * 4, 0, GL_STREAM_READ);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		WriteColorBufferA();
	break;

	case CELL_GCM_SURFACE_TARGET_1:
		glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[1]);
		glBufferData(GL_PIXEL_PACK_BUFFER, RSXThread::m_width * RSXThread::m_height * 4, 0, GL_STREAM_READ);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		WriteColorBufferB();
	break;

	case CELL_GCM_SURFACE_TARGET_MRT1:
		for (int i = 0; i < 2; i++)
		{
			glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[i]);
			glBufferData(GL_PIXEL_PACK_BUFFER, RSXThread::m_width * RSXThread::m_height * 4, 0, GL_STREAM_READ);
		}
		WriteColorBufferA();
		WriteColorBufferB();
	break;

	case CELL_GCM_SURFACE_TARGET_MRT2:
		for (int i = 0; i < 3; i++)
		{
			glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[i]);
			glBufferData(GL_PIXEL_PACK_BUFFER, RSXThread::m_width * RSXThread::m_height * 4, 0, GL_STREAM_READ);
		}
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		WriteColorBufferA();
		WriteColorBufferB();
		WriteColorBufferC();
	break;

	case CELL_GCM_SURFACE_TARGET_MRT3:
		for (int i = 0; i < 4; i++)
		{
			glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[i]);
			glBufferData(GL_PIXEL_PACK_BUFFER, RSXThread::m_width * RSXThread::m_height * 4, 0, GL_STREAM_READ);
		}
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		WriteColorBufferA();
		WriteColorBufferB();
		WriteColorBufferC();
		WriteColorBufferD();
	break;
	}
}

void GLGSRender::OnInit()
{
	m_draw_frames = 1;
	m_skip_frames = 0;
	RSXThread::m_width = 720;
	RSXThread::m_height = 576;
	RSXThread::m_width_scale = 2.0f;
	RSXThread::m_height_scale = 2.0f;

	last_width = 0;
	last_height = 0;
	last_depth_format = 0;

	m_frame->Show();
}

void GLGSRender::OnInitThread()
{
	m_context = m_frame->GetNewContext();

	m_frame->SetCurrent(m_context);

	InitProcTable();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

	glGenTextures(1, &g_depth_tex);
	glGenTextures(1, &g_flip_tex);
	glGenBuffers(6, g_pbo); // 4 for color buffers + 1 for depth buffer + 1 for flip()

#ifdef _WIN32
	glSwapInterval(Ini.GSVSyncEnable.GetValue() ? 1 : 0);
// Undefined reference: glXSwapIntervalEXT
/*#else
	if (GLXDrawable drawable = glXGetCurrentDrawable()){
		glXSwapIntervalEXT(glXGetCurrentDisplay(), drawable, Ini.GSVSyncEnable.GetValue() ? 1 : 0);
	}*/
#endif

}

void GLGSRender::OnExitThread()
{
	glDeleteTextures(1, &g_flip_tex);
	glDeleteTextures(1, &g_depth_tex);
	glDeleteBuffers(6, g_pbo);
	
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);

	m_program.Delete();
	m_rbo.Delete();
	m_fbo.Delete();
	m_vbo.Delete();
	m_vao.Delete();
	m_prog_buffer.Clear();
}

void GLGSRender::OnReset()
{
	m_program.UnUse();

	//m_shader_prog.id = 0;
	//m_vertex_prog.id = 0;

	if(m_vbo.IsCreated())
	{
		m_vbo.UnBind();
		m_vbo.Delete();
	}

	m_vao.Delete();
}

void GLGSRender::InitDrawBuffers()
{
	if(!m_fbo.IsCreated() || RSXThread::m_width != last_width || RSXThread::m_height != last_height || last_depth_format != m_surface_depth_format)
	{
		LOG_WARNING(RSX, "New FBO (%dx%d)", RSXThread::m_width, RSXThread::m_height);
		last_width = RSXThread::m_width;
		last_height = RSXThread::m_height;
		last_depth_format = m_surface_depth_format;

		m_fbo.Create();
		checkForGlError("m_fbo.Create");
		m_fbo.Bind();

		m_rbo.Create(4 + 1);
		checkForGlError("m_rbo.Create");

		for(int i=0; i<4; ++i)
		{
			m_rbo.Bind(i);
			m_rbo.Storage(GL_RGBA, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_RGBA)");
		}

		m_rbo.Bind(4);

		switch(m_surface_depth_format)
		{
		// case 0 found in BLJM60410-[Suzukaze no Melt - Days in the Sanctuary]  
		// [E : RSXThread]: Bad depth format! (0)
		// [E : RSXThread]: glEnable: opengl error 0x0506
		// [E : RSXThread]: glDrawArrays: opengl error 0x0506
		case 0:
			m_rbo.Storage(GL_DEPTH_COMPONENT, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_DEPTH_COMPONENT)");
		break;
		
		case CELL_GCM_SURFACE_Z16:
			m_rbo.Storage(GL_DEPTH_COMPONENT16, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_DEPTH_COMPONENT16)");
		break;

		case CELL_GCM_SURFACE_Z24S8:
			m_rbo.Storage(GL_DEPTH24_STENCIL8, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_DEPTH24_STENCIL8)");
		break;

		default:
			LOG_ERROR(RSX, "Bad depth format! (%d)", m_surface_depth_format);
			assert(0);
		break;
		}

		for(int i=0; i<4; ++i)
		{
			m_fbo.Renderbuffer(GL_COLOR_ATTACHMENT0 + i, m_rbo.GetId(i));
			checkForGlError(fmt::Format("m_fbo.Renderbuffer(GL_COLOR_ATTACHMENT%d)", i));
		}

		m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT, m_rbo.GetId(4));
		checkForGlError("m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT)");

		if(m_surface_depth_format == 2)
		{
			m_fbo.Renderbuffer(GL_STENCIL_ATTACHMENT, m_rbo.GetId(4));
			checkForGlError("m_fbo.Renderbuffer(GL_STENCIL_ATTACHMENT)");
		}
	}

	if(!m_set_surface_clip_horizontal)
	{
		m_surface_clip_x = 0;
		m_surface_clip_w = RSXThread::m_width;
	}

	if(!m_set_surface_clip_vertical)
	{
		m_surface_clip_y = 0;
		m_surface_clip_h = RSXThread::m_height;
	}
		
	m_fbo.Bind();

	static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };

	switch(m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_NONE:
		break;

	case CELL_GCM_SURFACE_TARGET_0:
		glDrawBuffer(draw_buffers[0]);
	break;

	case CELL_GCM_SURFACE_TARGET_1:
		glDrawBuffer(draw_buffers[1]);
	break;

	case CELL_GCM_SURFACE_TARGET_MRT1:
		glDrawBuffers(2, draw_buffers);
	break;

	case CELL_GCM_SURFACE_TARGET_MRT2:
		glDrawBuffers(3, draw_buffers);
	break;

	case CELL_GCM_SURFACE_TARGET_MRT3:
		glDrawBuffers(4, draw_buffers);
	break;

	checkForGlError("glDrawBuffers");

	default:
		LOG_ERROR(RSX, "Bad surface color target: %d", m_surface_color_target);
	break;
	}
}

void GLGSRender::ExecCMD(u32 cmd)
{
	assert(cmd == NV4097_CLEAR_SURFACE);
	
	InitDrawBuffers();

	if(m_set_color_mask)
	{
		glColorMask(m_color_mask_r, m_color_mask_g, m_color_mask_b, m_color_mask_a);
		checkForGlError("glColorMask");
	}

	if (m_set_scissor_horizontal && m_set_scissor_vertical)
	{
		glScissor(m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);
		checkForGlError("glScissor");
	}

	GLbitfield f = 0;

	if (m_clear_surface_mask & 0x1)
	{
		glClearDepth(m_clear_surface_z / (float)0xffffff);
		checkForGlError("glClearDepth");

		f |= GL_DEPTH_BUFFER_BIT;
	}

	if (m_clear_surface_mask & 0x2)
	{
		glClearStencil(m_clear_surface_s);
		checkForGlError("glClearStencil");

		f |= GL_STENCIL_BUFFER_BIT;
	}

	if (m_clear_surface_mask & 0xF0)
	{
		glClearColor(
			m_clear_surface_color_r / 255.0f,
			m_clear_surface_color_g / 255.0f,
			m_clear_surface_color_b / 255.0f,
			m_clear_surface_color_a / 255.0f);
		checkForGlError("glClearColor");

		f |= GL_COLOR_BUFFER_BIT;
	}

	glClear(f);
	checkForGlError("glClear");
	
	WriteBuffers();
}

void GLGSRender::ExecCMD()
{
	//return;
	if(!LoadProgram())
	{
		LOG_ERROR(RSX, "LoadProgram failed.");
		Emu.Pause();
		return;
	}

	InitDrawBuffers();

	if (m_set_color_mask)
	{
		glColorMask(m_color_mask_r, m_color_mask_g, m_color_mask_b, m_color_mask_a);
		checkForGlError("glColorMask");
	}

	Enable(m_set_depth_test, GL_DEPTH_TEST);
	Enable(m_set_alpha_test, GL_ALPHA_TEST);
	Enable(m_set_depth_bounds_test, GL_DEPTH_BOUNDS_TEST_EXT);
	Enable(m_set_blend || m_set_blend_mrt1 || m_set_blend_mrt2 || m_set_blend_mrt3, GL_BLEND);
	Enable(m_set_scissor_horizontal && m_set_scissor_vertical, GL_SCISSOR_TEST);
	Enable(m_set_logic_op, GL_LOGIC_OP);
	Enable(m_set_cull_face, GL_CULL_FACE);
	Enable(m_set_dither, GL_DITHER);
	Enable(m_set_stencil_test, GL_STENCIL_TEST);
	Enable(m_set_line_smooth, GL_LINE_SMOOTH);
	Enable(m_set_poly_smooth, GL_POLYGON_SMOOTH);
	Enable(m_set_point_sprite_control, GL_POINT_SPRITE);
	Enable(m_set_specular, GL_LIGHTING);
	Enable(m_set_poly_offset_fill, GL_POLYGON_OFFSET_FILL);
	Enable(m_set_poly_offset_line, GL_POLYGON_OFFSET_LINE);
	Enable(m_set_poly_offset_point, GL_POLYGON_OFFSET_POINT);
	Enable(m_set_restart_index, GL_PRIMITIVE_RESTART);
	Enable(m_set_line_stipple, GL_LINE_STIPPLE);
	Enable(m_set_polygon_stipple, GL_POLYGON_STIPPLE);
	
	if(m_set_clip_plane)
	{
		Enable(m_clip_plane_0, GL_CLIP_PLANE0);
		Enable(m_clip_plane_1, GL_CLIP_PLANE1);
		Enable(m_clip_plane_2, GL_CLIP_PLANE2);
		Enable(m_clip_plane_3, GL_CLIP_PLANE3);
		Enable(m_clip_plane_4, GL_CLIP_PLANE4);
		Enable(m_clip_plane_5, GL_CLIP_PLANE5);

		checkForGlError("m_set_clip_plane");
	}

	checkForGlError("glEnable");

	if (m_set_front_polygon_mode)
	{
		glPolygonMode(GL_FRONT, m_front_polygon_mode);
		checkForGlError("glPolygonMode(Front)");
	}

	if (m_set_back_polygon_mode)
	{
		glPolygonMode(GL_BACK, m_back_polygon_mode);
		checkForGlError("glPolygonMode(Back)");
	}

	if (m_set_point_size)
	{
		glPointSize(m_point_size);
		checkForGlError("glPointSize");
	}

	if (m_set_poly_offset_mode)
	{
		glPolygonOffset(m_poly_offset_scale_factor, m_poly_offset_bias);
		checkForGlError("glPolygonOffset");
	}

	if (m_set_logic_op)
	{
		glLogicOp(m_logic_op);
		checkForGlError("glLogicOp");
	}

	if (m_set_scissor_horizontal && m_set_scissor_vertical)
	{
		glScissor(m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);
		checkForGlError("glScissor");
	}
	
	if(m_set_two_sided_stencil_test_enable)
	{
		if(m_set_stencil_fail && m_set_stencil_zfail && m_set_stencil_zpass)
		{
			glStencilOpSeparate(GL_FRONT, m_stencil_fail, m_stencil_zfail, m_stencil_zpass);
			checkForGlError("glStencilOpSeparate");
		}

		if(m_set_stencil_mask)
		{
			glStencilMaskSeparate(GL_FRONT, m_stencil_mask);
			checkForGlError("glStencilMaskSeparate");
		}

		if(m_set_stencil_func && m_set_stencil_func_ref && m_set_stencil_func_mask)
		{
			glStencilFuncSeparate(GL_FRONT, m_stencil_func, m_stencil_func_ref, m_stencil_func_mask);
			checkForGlError("glStencilFuncSeparate");
		}

		if(m_set_back_stencil_fail && m_set_back_stencil_zfail && m_set_back_stencil_zpass)
		{
			glStencilOpSeparate(GL_BACK, m_back_stencil_fail, m_back_stencil_zfail, m_back_stencil_zpass);
			checkForGlError("glStencilOpSeparate(GL_BACK)");
		}

		if(m_set_back_stencil_mask)
		{
			glStencilMaskSeparate(GL_BACK, m_back_stencil_mask);
			checkForGlError("glStencilMaskSeparate(GL_BACK)");
		}

		if(m_set_back_stencil_func && m_set_back_stencil_func_ref && m_set_back_stencil_func_mask)
		{
			glStencilFuncSeparate(GL_BACK, m_back_stencil_func, m_back_stencil_func_ref, m_back_stencil_func_mask);
			checkForGlError("glStencilFuncSeparate(GL_BACK)");
		}
	}
	else
	{
		if(m_set_stencil_fail && m_set_stencil_zfail && m_set_stencil_zpass)
		{
			glStencilOp(m_stencil_fail, m_stencil_zfail, m_stencil_zpass);
			checkForGlError("glStencilOp");
		}

		if(m_set_stencil_mask)
		{
			glStencilMask(m_stencil_mask);
			checkForGlError("glStencilMask");
		}

		if(m_set_stencil_func && m_set_stencil_func_ref && m_set_stencil_func_mask)
		{
			glStencilFunc(m_stencil_func, m_stencil_func_ref, m_stencil_func_mask);
			checkForGlError("glStencilFunc");
		}
	}

	if (m_set_two_side_light_enable)
	{
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
		checkForGlError("glLightModeli");
	}

	if(m_set_shade_mode)
	{
		glShadeModel(m_shade_mode);
		checkForGlError("glShadeModel");
	}

	if(m_set_depth_mask)
	{
		glDepthMask(m_depth_mask);
		checkForGlError("glDepthMask");
	}

	if(m_set_depth_func)
	{
		glDepthFunc(m_depth_func);
		checkForGlError("glDepthFunc");
	}

	if(m_set_depth_bounds)
	{
		glDepthBoundsEXT(m_depth_bounds_min, m_depth_bounds_max);
		checkForGlError("glDepthBounds");
	}

	if(m_set_clip)
	{
		glDepthRangef(m_clip_min, m_clip_max);
		checkForGlError("glDepthRangef");
	}

	if(m_set_line_width)
	{
		glLineWidth(m_line_width);
		checkForGlError("glLineWidth");
	}

	if (m_set_line_stipple)
	{
		glLineStipple(m_line_stipple_factor, m_line_stipple_pattern);
		checkForGlError("glLineStipple");
	}

	if (m_set_polygon_stipple)
	{
		glPolygonStipple((const GLubyte*)m_polygon_stipple_pattern);
		checkForGlError("glPolygonStipple");
	}
	
	if(m_set_blend_equation)
	{
		glBlendEquationSeparate(m_blend_equation_rgb, m_blend_equation_alpha);
		checkForGlError("glBlendEquationSeparate");
	}

	if(m_set_blend_sfactor && m_set_blend_dfactor)
	{
		glBlendFuncSeparate(m_blend_sfactor_rgb, m_blend_dfactor_rgb, m_blend_sfactor_alpha, m_blend_dfactor_alpha);
		checkForGlError("glBlendFuncSeparate");
	}

	if(m_set_blend_color)
	{
		glBlendColor(m_blend_color_r, m_blend_color_g, m_blend_color_b, m_blend_color_a);
		checkForGlError("glBlendColor");
	}

	if(m_set_cull_face)
	{
		glCullFace(m_cull_face);
		checkForGlError("glCullFace");
	}

	if (m_set_front_face)
	{
		glFrontFace(m_front_face);
		checkForGlError("glFrontFace");
	}

	if(m_set_alpha_func && m_set_alpha_ref)
	{
		glAlphaFunc(m_alpha_func, m_alpha_ref);
		checkForGlError("glAlphaFunc");
	}

	if(m_set_fog_mode)
	{
		glFogi(GL_FOG_MODE, m_fog_mode);
		checkForGlError("glFogi(GL_FOG_MODE)");
	}

	if(m_set_fog_params)
	{
		glFogf(GL_FOG_START, m_fog_param0);
		checkForGlError("glFogf(GL_FOG_START)");
		glFogf(GL_FOG_END, m_fog_param1);
		checkForGlError("glFogf(GL_FOG_END)");
	}

	if(m_set_restart_index)
	{
		glPrimitiveRestartIndex(m_restart_index);
		checkForGlError("glPrimitiveRestartIndex");
	}

	if(m_indexed_array.m_count && m_draw_array_count)
	{
		LOG_WARNING(RSX, "m_indexed_array.m_count && draw_array_count");
	}

	for(u32 i=0; i<m_textures_count; ++i)
	{
		if(!m_textures[i].IsEnabled()) continue;

		glActiveTexture(GL_TEXTURE0 + i);
		checkForGlError("glActiveTexture");
		m_gl_textures[i].Create();
		m_gl_textures[i].Bind();
		checkForGlError(fmt::Format("m_gl_textures[%d].Bind", i));
		m_program.SetTex(i);
		m_gl_textures[i].Init(m_textures[i]);
		checkForGlError(fmt::Format("m_gl_textures[%d].Init", i));
	}

	m_vao.Bind();
	if(m_indexed_array.m_count)
	{
		LoadVertexData(m_indexed_array.index_min, m_indexed_array.index_max - m_indexed_array.index_min + 1);
	}

	EnableVertexData(m_indexed_array.m_count ? true : false);

	InitVertexData();
	InitFragmentData();

	if(m_indexed_array.m_count)
	{
		switch(m_indexed_array.m_type)
		{
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32:
			glDrawElements(m_draw_mode - 1, m_indexed_array.m_count, GL_UNSIGNED_INT, nullptr);
			checkForGlError("glDrawElements #4");
		break;

		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
			glDrawElements(m_draw_mode - 1, m_indexed_array.m_count, GL_UNSIGNED_SHORT, nullptr);
			checkForGlError("glDrawElements #2");
		break;

		default:
			LOG_ERROR(RSX, "Bad indexed array type (%d)", m_indexed_array.m_type);
		break;
		}

		DisableVertexData();
		m_indexed_array.Reset();
	}

	if(m_draw_array_count)
	{
		//LOG_WARNING(RSX,"glDrawArrays(%d,%d,%d)", m_draw_mode - 1, m_draw_array_first, m_draw_array_count);
		glDrawArrays(m_draw_mode - 1, 0, m_draw_array_count);
		checkForGlError("glDrawArrays");
		DisableVertexData();
	}
	
	WriteBuffers();
}

void GLGSRender::Flip()
{
	// Set scissor to FBO size 
	if (m_set_scissor_horizontal && m_set_scissor_vertical)
	{
		glScissor(0, 0, RSXThread::m_width, RSXThread::m_height);
		checkForGlError("glScissor");
	}
	
	switch (m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_0:
	case CELL_GCM_SURFACE_TARGET_1:
	case CELL_GCM_SURFACE_TARGET_MRT1:
	case CELL_GCM_SURFACE_TARGET_MRT2:
	case CELL_GCM_SURFACE_TARGET_MRT3:
	{
		// Fast path for non-MRT using glBlitFramebuffer.
		GLfbo::Bind(GL_DRAW_FRAMEBUFFER, 0);
		// Renderbuffer is upside turn , swapped srcY0 and srcY1
		GLfbo::Blit(0, RSXThread::m_height, RSXThread::m_width, 0, 0, 0, RSXThread::m_width, RSXThread::m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}
	break;

	case CELL_GCM_SURFACE_TARGET_NONE:
	{
		// Slow path for MRT/None target using glReadPixels.
		static u8* src_buffer = nullptr;
		static u32 width = 0;
		static u32 height = 0;
		GLenum format = GL_RGBA;

		if (m_read_buffer)
		{
			format = GL_BGRA;
			CellGcmDisplayInfo* buffers = vm::get_ptr<CellGcmDisplayInfo>(m_gcm_buffers_addr);
			u32 addr = GetAddress(buffers[m_gcm_current_buffer].offset, CELL_GCM_LOCATION_LOCAL);

			if (Memory.IsGoodAddr(addr))
			{
				width = buffers[m_gcm_current_buffer].width;
				height = buffers[m_gcm_current_buffer].height;
				src_buffer = vm::get_ptr<u8>(addr);
			}
			else
			{
				src_buffer = nullptr;
			}
		}
		else if (m_fbo.IsCreated())
		{
			format = GL_RGBA;
			static std::vector<u8> pixels;
			pixels.resize(RSXThread::m_width * RSXThread::m_height * 4);
			m_fbo.Bind(GL_READ_FRAMEBUFFER);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[5]);
			glBufferData(GL_PIXEL_PACK_BUFFER, RSXThread::m_width * RSXThread::m_height * 4, 0, GL_STREAM_READ);
			glReadPixels(0, 0, RSXThread::m_width, RSXThread::m_height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, 0);
			checkForGlError("Flip(): glReadPixels(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8)");
			GLubyte *packed = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
			memcpy(pixels.data(), packed, RSXThread::m_width * RSXThread::m_height * 4);
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

			src_buffer = pixels.data();
			width = RSXThread::m_width;
			height = RSXThread::m_height;
		}
		else
			src_buffer = nullptr;

		if (src_buffer)
		{
			glDisable(GL_STENCIL_TEST);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CLIP_PLANE0);
			glDisable(GL_CLIP_PLANE1);
			glDisable(GL_CLIP_PLANE2);
			glDisable(GL_CLIP_PLANE3);
			glDisable(GL_CLIP_PLANE4);
			glDisable(GL_CLIP_PLANE5);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, g_flip_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_INT_8_8_8_8, src_buffer);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, 1, 0, 1, 0, 1);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			GLfbo::Bind(GL_DRAW_FRAMEBUFFER, 0);

			m_program.UnUse();
			m_program.Use();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);

			glColor3f(1, 1, 1);
			glBegin(GL_QUADS);
			glTexCoord2i(0, 1);
			glVertex2i(0, 0);

			glTexCoord2i(1, 1);
			glVertex2i(1, 0);

			glTexCoord2i(1, 0);
			glVertex2i(1, 1);

			glTexCoord2i(0, 0);
			glVertex2i(0, 1);
			glEnd();
		}
	}
	break;
	}

	// Draw Objects
	for (uint i = 0; i<m_post_draw_objs.size(); ++i)
	{
		m_post_draw_objs[i].Draw();
	}

	m_frame->Flip(m_context);
	
	// Restore scissor
	if (m_set_scissor_horizontal && m_set_scissor_vertical)
	{
		glScissor(m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);
		checkForGlError("glScissor");
	}

}

u32 LinearToSwizzleAddress(u32 x, u32 y, u32 z, u32 log2_width, u32 log2_height, u32 log2_depth)
{
	u32 offset = 0;
	u32 shift_count = 0;
	while(log2_width | log2_height | log2_depth){
		if(log2_width){
			offset |= (x & 0x01) << shift_count;
			x >>= 1;
			++shift_count;
			--log2_width;
		}
		if(log2_height){
			offset |= (y & 0x01) << shift_count;
			y >>= 1;
			++shift_count;
			--log2_height;
		}
		if(log2_depth){
			offset |= (z & 0x01) << shift_count;
			z >>= 1;
			++shift_count;
			--log2_depth;
		}
	}
	return offset;
}
