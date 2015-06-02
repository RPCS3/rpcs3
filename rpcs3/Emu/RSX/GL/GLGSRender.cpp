#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/rPlatform.h" // only for rImage
#include "Utilities/File.h"
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
	if (err != GL_NO_ERROR)
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
	case CELL_GCM_TEXTURE_CLAMP: return GL_CLAMP;
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

void GLTexture::Init(rsx::texture& tex)
{
	if (tex.location() > 1)
	{
		return;
	}

	Bind();

	const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
	//LOG_WARNING(RSX, "texture addr = 0x%x, width = %d, height = %d, max_aniso=%d, mipmap=%d, remap=0x%x, zfunc=0x%x, wraps=0x%x, wrapt=0x%x, wrapr=0x%x, minlod=0x%x, maxlod=0x%x", 
	//	m_offset, m_width, m_height, m_maxaniso, m_mipmap, m_remap, m_zfunc, m_wraps, m_wrapt, m_wrapr, m_minlod, m_maxlod);
	
	//TODO: safe init
	checkForGlError("GLTexture::Init() -> glBindTexture");

	int format = tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
	bool is_swizzled = !(tex.format() & CELL_GCM_TEXTURE_LN);

	auto pixels = vm::get_ptr<const u8>(texaddr);
	u8 *unswizzledPixels;
	static const GLint glRemapStandard[4] = { GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE };
	// NOTE: This must be in ARGB order in all forms below.
	const GLint *glRemap = glRemapStandard;

	switch (format)
	{
	case CELL_GCM_TEXTURE_B8: // One 8-bit fixed-point number
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BLUE, GL_UNSIGNED_BYTE, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_B8)");

		static const GLint swizzleMaskB8[] = { GL_BLUE, GL_BLUE, GL_BLUE, GL_BLUE };
		glRemap = swizzleMaskB8;
		break;
	}

	case CELL_GCM_TEXTURE_A1R5G5B5:
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
		checkForGlError("GLTexture::Init() -> glPixelStorei");

		// TODO: texture swizzling
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_A1R5G5B5)");

		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
		checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_A1R5G5B5)");
		break;
	}

	case CELL_GCM_TEXTURE_A4R4G4B4:
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_A4R4G4B4)");

		// We read it in as R4G4B4A4, so we need to remap each component.
		static const GLint swizzleMaskA4R4G4B4[] = { GL_BLUE, GL_ALPHA, GL_RED, GL_GREEN };
		glRemap = swizzleMaskA4R4G4B4;
		break;
	}

	case CELL_GCM_TEXTURE_R5G6B5:
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
		checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_R5G6B5)");

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex.width(), tex.height(), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_R5G6B5)");

		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
		checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_R5G6B5)");
		break;
	}

	case CELL_GCM_TEXTURE_A8R8G8B8:
	{
		if (is_swizzled)
		{
			u32 *src, *dst;
			u32 log2width, log2height;

			unswizzledPixels = (u8*)malloc(tex.width() * tex.height() * 4);
			src = (u32*)pixels;
			dst = (u32*)unswizzledPixels;

			log2width = log(tex.width()) / log(2);
			log2height = log(tex.height()) / log(2);

			for (int i = 0; i < tex.height(); i++)
			{
				for (int j = 0; j < tex.width(); j++)
				{
					dst[(i*tex.height()) + j] = src[rsx::linear_to_swizzle(j, i, 0, log2width, log2height, 0)];
				}
			}
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, is_swizzled ? unswizzledPixels : pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_A8R8G8B8)");
		break;
	}

	case CELL_GCM_TEXTURE_COMPRESSED_DXT1: // Compressed 4x4 pixels into 8 bytes
	{
		u32 size = ((tex.width() + 3) / 4) * ((tex.height() + 3) / 4) * 8;

		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, tex.width(), tex.height(), 0, size, pixels);
		checkForGlError("GLTexture::Init() -> glCompressedTexImage2D(CELL_GCM_TEXTURE_COMPRESSED_DXT1)");
		break;
	}

	case CELL_GCM_TEXTURE_COMPRESSED_DXT23: // Compressed 4x4 pixels into 16 bytes
	{
		u32 size = ((tex.width() + 3) / 4) * ((tex.height() + 3) / 4) * 16;

		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, tex.width(), tex.height(), 0, size, pixels);
		checkForGlError("GLTexture::Init() -> glCompressedTexImage2D(CELL_GCM_TEXTURE_COMPRESSED_DXT23)");
	}
		break;

	case CELL_GCM_TEXTURE_COMPRESSED_DXT45: // Compressed 4x4 pixels into 16 bytes
	{
		u32 size = ((tex.width() + 3) / 4) * ((tex.height() + 3) / 4) * 16;

		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, tex.width(), tex.height(), 0, size, pixels);
		checkForGlError("GLTexture::Init() -> glCompressedTexImage2D(CELL_GCM_TEXTURE_COMPRESSED_DXT45)");
		break;
	}

	case CELL_GCM_TEXTURE_G8B8:
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RG, GL_UNSIGNED_BYTE, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_G8B8)");

		static const GLint swizzleMaskG8B8[] = { GL_RED, GL_GREEN, GL_RED, GL_GREEN };
		glRemap = swizzleMaskG8B8;
		break;
	}

	case CELL_GCM_TEXTURE_R6G5B5:
	{
		// TODO: Probably need to actually unswizzle if is_swizzled.
		const u32 numPixels = tex.width() * tex.height();
		unswizzledPixels = (u8 *)malloc(numPixels * 4);
		// TODO: Speed.
		for (u32 i = 0; i < numPixels; ++i) {
			u16 c = reinterpret_cast<const be_t<u16> *>(pixels)[i];
			unswizzledPixels[i * 4 + 0] = Convert6To8((c >> 10) & 0x3F);
			unswizzledPixels[i * 4 + 1] = Convert5To8((c >> 5) & 0x1F);
			unswizzledPixels[i * 4 + 2] = Convert5To8((c >> 0) & 0x1F);
			unswizzledPixels[i * 4 + 3] = 255;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, unswizzledPixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_R6G5B5)");

		free(unswizzledPixels);
		break;
	}

	case CELL_GCM_TEXTURE_DEPTH24_D8: //  24-bit unsigned fixed-point number and 8 bits of garbage
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_DEPTH24_D8)");
		break;
	}

	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: // 24-bit unsigned float and 8 bits of garbage
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT)");
		break;
	}

	case CELL_GCM_TEXTURE_DEPTH16: // 16-bit unsigned fixed-point number
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_SHORT, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_DEPTH16)");
		break;
	}

	case CELL_GCM_TEXTURE_DEPTH16_FLOAT: // 16-bit unsigned float
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_DEPTH16_FLOAT)");
		break;
	}

	case CELL_GCM_TEXTURE_X16: // A 16-bit fixed-point number
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
		checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_X16)");

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RED, GL_UNSIGNED_SHORT, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_X16)");

		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
		checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_X16)");

		static const GLint swizzleMaskX16[] = { GL_RED, GL_ONE, GL_RED, GL_ONE };
		glRemap = swizzleMaskX16;
		break;
	}

	case CELL_GCM_TEXTURE_Y16_X16: // Two 16-bit fixed-point numbers
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
		checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_Y16_X16)");

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RG, GL_UNSIGNED_SHORT, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_Y16_X16)");

		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
		checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_Y16_X16)");

		static const GLint swizzleMaskX32_Y16_X16[] = { GL_GREEN, GL_RED, GL_GREEN, GL_RED };
		glRemap = swizzleMaskX32_Y16_X16;
		break;
	}

	case CELL_GCM_TEXTURE_R5G5B5A1:
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
		checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_R5G5B5A1)");

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_R5G5B5A1)");

		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
		checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_R5G5B5A1)");
		break;
	}

	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: // Four fp16 values
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
		checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT)");

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_HALF_FLOAT, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT)");

		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
		checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT)");
		break;
	}

	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: // Four fp32 values
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_FLOAT, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT)");
		break;
	}

	case CELL_GCM_TEXTURE_X32_FLOAT: // One 32-bit floating-point number
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RED, GL_FLOAT, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_X32_FLOAT)");

		static const GLint swizzleMaskX32_FLOAT[] = { GL_RED, GL_ONE, GL_ONE, GL_ONE };
		glRemap = swizzleMaskX32_FLOAT;
		break;
	}

	case CELL_GCM_TEXTURE_D1R5G5B5:
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
		checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_D1R5G5B5)");

		// TODO: Texture swizzling
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_D1R5G5B5)");

		static const GLint swizzleMaskX32_D1R5G5B5[] = { GL_ONE, GL_RED, GL_GREEN, GL_BLUE };
		glRemap = swizzleMaskX32_D1R5G5B5;

		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
		checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_D1R5G5B5)");
		break;
	}

	case CELL_GCM_TEXTURE_D8R8G8B8: // 8 bits of garbage and three unsigned 8-bit fixed-point numbers
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_D8R8G8B8)");

		static const GLint swizzleMaskX32_D8R8G8B8[] = { GL_ONE, GL_RED, GL_GREEN, GL_BLUE };
		glRemap = swizzleMaskX32_D8R8G8B8;
		break;
	}


	case CELL_GCM_TEXTURE_Y16_X16_FLOAT: // Two fp16 values
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
		checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_Y16_X16_FLOAT)");

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RG, GL_HALF_FLOAT, pixels);
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_Y16_X16_FLOAT)");

		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
		checkForGlError("GLTexture::Init() -> glPixelStorei(CELL_GCM_TEXTURE_Y16_X16_FLOAT)");

		static const GLint swizzleMaskX32_Y16_X16_FLOAT[] = { GL_RED, GL_GREEN, GL_RED, GL_GREEN };
		glRemap = swizzleMaskX32_Y16_X16_FLOAT;
		break;
	}

	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	{
		const u32 numPixels = tex.width() * tex.height();
		unswizzledPixels = (u8 *)malloc(numPixels * 4);
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
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8 & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN)");

		free(unswizzledPixels);
		break;
	}

	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
	{
		const u32 numPixels = tex.width() * tex.height();
		unswizzledPixels = (u8 *)malloc(numPixels * 4);
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
		checkForGlError("GLTexture::Init() -> glTexImage2D(CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8 & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN)");

		free(unswizzledPixels);
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

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GetGlWrap(tex.wrap_s()));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GetGlWrap(tex.wrap_t()));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GetGlWrap(tex.wrap_r()));

	checkForGlError("GLTexture::Init() -> wrap");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, gl_tex_zfunc[tex.zfunc()]);

	checkForGlError("GLTexture::Init() -> compare");

	glTexEnvi(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, tex.bias());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, (tex.min_lod() >> 8));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, (tex.max_lod() >> 8));

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

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_tex_min_filter[tex.min_filter()]);

	checkForGlError("GLTexture::Init() -> min filters");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_mag_filter[tex.mag_filter()]);

	checkForGlError("GLTexture::Init() -> mag filters");
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, GetMaxAniso(tex.max_aniso()));

	checkForGlError("GLTexture::Init() -> max anisotropy");

	//Unbind();

	if (is_swizzled && format == CELL_GCM_TEXTURE_A8R8G8B8)
	{
		free(unswizzledPixels);
	}
}

void GLTexture::Save(rsx::texture& tex, const std::string& name)
{
	if (!m_id || !tex.offset() || !tex.width() || !tex.height()) return;

	const u32 texPixelCount = tex.width() * tex.height();

	u32* alldata = new u32[texPixelCount];

	Bind();

	switch (tex.format() & ~(0x20 | 0x40))
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

	fs::file(name + ".raw", o_write | o_create | o_trunc).write(alldata, texPixelCount * 4);

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
	out.Create(tex.width(), tex.height(), data, alpha);
	out.SaveFile(name, rBITMAP_TYPE_PNG);

	delete[] alldata;
	//free(data);
	//free(alpha);
}

void GLTexture::Save(rsx::texture& tex)
{
	static const std::string& dir_path = "textures";
	static const std::string& file_fmt = dir_path + "/" + "tex[%d].png";

	if (!fs::exists(dir_path)) fs::create_dir(dir_path);

	u32 count = 0;
	while (fs::exists(fmt::Format(file_fmt.c_str(), count))) count++;
	Save(tex, fmt::Format(file_fmt.c_str(), count));
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
	m_program.Create(m_vp.id, m_fp.id);
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
		glUniform4f(m_program.GetLocation("in_pos"), m_pos_x, m_pos_y, m_pos_z, 1.0f);
		checkForGlError("DrawCursorObj : glUniform4f");
	}

	glDrawArrays(GL_QUADS, 0, 4);
	checkForGlError("DrawCursorObj : glDrawArrays");

	m_fbo.Bind(GL_READ_FRAMEBUFFER);
	checkForGlError("DrawCursorObj : m_fbo.Bind(GL_READ_FRAMEBUFFER)");
	GLfbo::Bind(GL_DRAW_FRAMEBUFFER, 0);
	checkForGlError("DrawCursorObj : GLfbo::Bind(GL_DRAW_FRAMEBUFFER, 0)");
	GLfbo::Blit(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	checkForGlError("DrawCursorObj : GLfbo::Blit");
	m_fbo.Bind();
	checkForGlError("DrawCursorObj : m_fbo.Bind");
}

void DrawCursorObj::InitializeShaders()
{
	m_vp.shader =
		"#version 420\n"
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

	m_fp.shader = 
		"#version 420\n"
		"\n"
		"in vec2 tc;\n"
		"layout (binding = 0) uniform sampler2D tex0;\n"
		"layout (location = 0) out vec4 res;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	res = texture(tex0, tc);\n"
		"}\n";
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
	//LOG_WARNING(RSX, "tex0 location = 0x%x", m_program.GetLocation("tex0"));
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
	if (enable)
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

	if (m_frame->IsShown())
	{
		m_frame->Hide();
	}
	m_ctrl = nullptr;
}

void GLGSRender::EnableVertexData(bool indexed_draw)
{
	/*
	static u32 offset_list[m_vertex_count];
	u32 cur_offset = 0;

	const u32 data_offset = indexed_draw ? 0 : m_draw_array_first;

	for (u32 i = 0; i < m_vertex_count; ++i)
	{
		if (0)
		{
			u32 data_format = methodRegisters[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + i * 4];
			u16 frequency = data_format >> 16;
			u8 stride = (data_format >> 8) & 0xff;
			u8 size = (data_format >> 4) & 0xf;
			u8 type = data_format & 0xf;

			u32 type_size = 1;
			switch (type)
			{
			case CELL_GCM_VERTEX_S1:    type_size = 2; break;
			case CELL_GCM_VERTEX_F:     type_size = 4; break;
			case CELL_GCM_VERTEX_SF:    type_size = 2; break;
			case CELL_GCM_VERTEX_UB:    type_size = 1; break;
			case CELL_GCM_VERTEX_S32K:  type_size = 2; break;
			case CELL_GCM_VERTEX_CMP:   type_size = 4; break;
			case CELL_GCM_VERTEX_UB256: type_size = 1; break;

			default:
				LOG_ERROR(RSX, "RSXVertexData::GetTypeSize: Bad vertex data type (%d)!", type);
				break;
			}

			int item_size = size * type_size;
		}

		offset_list[i] = cur_offset;

		if (!m_vertex_data[i].IsEnabled()) continue;
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
	m_vbo.SetData(m_vdata.data(), m_vdata.size());

	if (indexed_draw)
	{
		m_vbo.Bind(GL_ELEMENT_ARRAY_BUFFER, 1);
		m_vbo.SetData(GL_ELEMENT_ARRAY_BUFFER, m_indexed_array.m_data.data(), m_indexed_array.m_data.size());
	}

	checkForGlError("initializing vbo");

#if	DUMP_VERTEX_DATA
	rFile dump("VertexDataArray.dump", rFile::write);
#endif

	for (u32 i = 0; i < m_vertex_count; ++i)
	{
		if (!m_vertex_data[i].IsEnabled()) continue;

#if	DUMP_VERTEX_DATA
		dump.Write(wxString::Format("VertexData[%d]:\n", i));
		switch (m_vertex_data[i].type)
		{
		case CELL_GCM_VERTEX_S1:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 2)
			{
				dump.Write(wxString::Format("%d\n", *(u16*)&m_vertex_data[i].data[j]));
				if (!(((j + 2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_F:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 4)
			{
				dump.Write(wxString::Format("%.01f\n", *(float*)&m_vertex_data[i].data[j]));
				if (!(((j + 4) / 4) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_SF:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 2)
			{
				dump.Write(wxString::Format("%.01f\n", *(float*)&m_vertex_data[i].data[j]));
				if (!(((j + 2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_UB:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); ++j)
			{
				dump.Write(wxString::Format("%d\n", m_vertex_data[i].data[j]));
				if (!((j + 1) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_S32K:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 2)
			{
				dump.Write(wxString::Format("%d\n", *(u16*)&m_vertex_data[i].data[j]));
				if (!(((j + 2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

			// case CELL_GCM_VERTEX_CMP:

		case CELL_GCM_VERTEX_UB256:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); ++j)
			{
				dump.Write(wxString::Format("%d\n", m_vertex_data[i].data[j]));
				if (!((j + 1) % m_vertex_data[i].size)) dump.Write("\n");
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

		if (m_vertex_data[i].type < 1 || m_vertex_data[i].type > 7)
		{
			LOG_ERROR(RSX, "GLGSRender::EnableVertexData: Bad vertex data type (%d)!", m_vertex_data[i].type);
		}

		if (!m_vertex_data[i].addr)
		{
			switch (m_vertex_data[i].type)
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
				switch (m_vertex_data[i].size)
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
	}*/
}

void GLGSRender::DisableVertexData()
{
	m_vdata.clear();
	for (u32 i = 0; i < m_vertex_count; ++i)
	{
		if (!m_vertex_data[i].IsEnabled()) continue;
		glDisableVertexAttribArray(i);
		checkForGlError("glDisableVertexAttribArray");
	}
	m_vao.Unbind();
}

void GLGSRender::InitVertexData()
{
	/*
	int l;
	GLfloat scaleOffsetMat[16] =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	for (const RSXTransformConstant& c : m_transform_constants)
	{
		const std::string name = fmt::Format("vc[%u]", c.id);
		l = m_program.GetLocation(name);
		checkForGlError("glGetUniformLocation " + name);

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
	*/
}

void GLGSRender::InitFragmentData()
{
	/*
	if (!m_cur_fragment_prog)
	{
		LOG_ERROR(RSX, "InitFragmentData: m_cur_shader_prog == NULL");
		return;
	}

	// Get constant from fragment program
	const std::vector<size_t> &fragmentOffset = m_prog_buffer.getFragmentConstantOffsetsCache(m_cur_fragment_prog);
	for (size_t offsetInFP : fragmentOffset)
	{
		auto data = vm::ptr<u32>::make(m_cur_fragment_prog->addr + (u32)offsetInFP);

		u32 c0 = (data[0] >> 16 | data[0] << 16);
		u32 c1 = (data[1] >> 16 | data[1] << 16);
		u32 c2 = (data[2] >> 16 | data[2] << 16);
		u32 c3 = (data[3] >> 16 | data[3] << 16);
		const std::string name = fmt::Format("fc%u", offsetInFP);
		const int l = m_program.GetLocation(name);
		checkForGlError("glGetUniformLocation " + name);

		float f0 = (float&)c0;
		float f1 = (float&)c1;
		float f2 = (float&)c2;
		float f3 = (float&)c3;
		glUniform4f(l, f0, f1, f2, f3);
		checkForGlError("glUniform4f " + name + fmt::Format(" %u [%f %f %f %f]", l, f0, f1, f2, f3));
	}

	for (const RSXTransformConstant& c : m_fragment_constants)
	{
		u32 id = c.id - m_cur_fragment_prog->offset;

		//LOG_WARNING(RSX,"fc%u[0x%x - 0x%x] = (%f, %f, %f, %f)", id, c.id, m_cur_shader_prog->offset, c.x, c.y, c.z, c.w);

		const std::string name = fmt::Format("fc%u", id);
		const int l = m_program.GetLocation(name);
		checkForGlError("glGetUniformLocation " + name);

		glUniform4f(l, c.x, c.y, c.z, c.w);
		checkForGlError("glUniform4f " + name + fmt::Format(" %u [%f %f %f %f]", l, c.x, c.y, c.z, c.w));
	}


	//if (m_fragment_constants.GetCount())
	//	LOG_NOTICE(HLE, "");
	*/
}

bool GLGSRender::LoadProgram()
{
	/*
	if (!m_cur_fragment_prog)
	{
		LOG_WARNING(RSX, "LoadProgram: m_cur_shader_prog == NULL");
		return false;
	}

	m_cur_fragment_prog->ctrl = m_shader_ctrl;

	if (!m_cur_vertex_prog)
	{
		LOG_WARNING(RSX, "LoadProgram: m_cur_vertex_prog == NULL");
		return false;
	}

	GLProgram *result = m_prog_buffer.getGraphicPipelineState(m_cur_vertex_prog, m_cur_fragment_prog, nullptr, nullptr);
	m_program.id = result->id;
	m_program.Use();
	*/
	return true;
}

void GLGSRender::WriteBuffers()
{
	/*
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
	*/
}

void GLGSRender::WriteDepthBuffer()
{
	/*
	if (!m_set_context_dma_z)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_z, m_context_dma_z - 0xfeed0000);

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
	*/
}

void GLGSRender::WriteColorBufferA()
{
	/*
	if (!m_set_context_dma_color_a)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_a, m_context_dma_color_a - 0xfeed0000);

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
	*/
}

void GLGSRender::WriteColorBufferB()
{
	/*
	if (!m_set_context_dma_color_b)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_b, m_context_dma_color_b - 0xfeed0000);

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
	*/
}

void GLGSRender::WriteColorBufferC()
{
	/*
	if (!m_set_context_dma_color_c)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_c, m_context_dma_color_c - 0xfeed0000);

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
	*/
}

void GLGSRender::WriteColorBufferD()
{
	/*
	if (!m_set_context_dma_color_d)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_d, m_context_dma_color_d - 0xfeed0000);

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
	*/
}

void GLGSRender::WriteColorBuffers()
{
	/*
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
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
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
	*/
}

void GLGSRender::oninit()
{
	m_draw_frames = 1;
	m_skip_frames = 0;

	last_width = 0;
	last_height = 0;
	last_depth_format = 0;

	m_frame->Show();
}

void GLGSRender::oninit_thread()
{
	m_context = m_frame->GetNewContext();
	
	m_frame->SetCurrent(m_context);

	InitProcTable();

	is_intel_vendor = strstr((const char*)glGetString(GL_VENDOR), "Intel");
	
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

	glGenTextures(1, &g_depth_tex);
	glGenTextures(1, &g_flip_tex);
	glGenBuffers(6, g_pbo); // 4 for color buffers + 1 for depth buffer + 1 for flip()

#ifdef _WIN32
	glSwapInterval(Ini.GSVSyncEnable.GetValue() ? 1 : 0);
#endif

}

void GLGSRender::onexit_thread()
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
}

void GLGSRender::onreset()
{
	m_program.UnUse();

	if (m_vbo.IsCreated())
	{
		m_vbo.UnBind();
		m_vbo.Delete();
	}

	m_vao.Delete();
}

void GLGSRender::InitDrawBuffers()
{
	/*
	if (!m_fbo.IsCreated() || RSXThread::m_width != last_width || RSXThread::m_height != last_height || last_depth_format != m_surface_depth_format)
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

		for (int i = 0; i < 4; ++i)
		{
			m_rbo.Bind(i);
			m_rbo.Storage(GL_RGBA, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_RGBA)");
		}

		m_rbo.Bind(4);

		switch (m_surface_depth_format)
		{
		case 0:
		{
			// case 0 found in BLJM60410-[Suzukaze no Melt - Days in the Sanctuary]
			// [E : RSXThread]: Bad depth format! (0)
			// [E : RSXThread]: glEnable: opengl error 0x0506
			// [E : RSXThread]: glDrawArrays: opengl error 0x0506
			m_rbo.Storage(GL_DEPTH_COMPONENT, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_DEPTH_COMPONENT)");
			break;
		}

		case CELL_GCM_SURFACE_Z16:
		{
			m_rbo.Storage(GL_DEPTH_COMPONENT16, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_DEPTH_COMPONENT16)");

			m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT, m_rbo.GetId(4));
			checkForGlError("m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT)");
			break;
		}
			

		case CELL_GCM_SURFACE_Z24S8:
		{
			m_rbo.Storage(GL_DEPTH24_STENCIL8, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_DEPTH24_STENCIL8)");

			m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT, m_rbo.GetId(4));
			checkForGlError("m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT)");

			m_fbo.Renderbuffer(GL_STENCIL_ATTACHMENT, m_rbo.GetId(4));
			checkForGlError("m_fbo.Renderbuffer(GL_STENCIL_ATTACHMENT)");

			break;

		}
			

		default:
		{
			LOG_ERROR(RSX, "Bad depth format! (%d)", m_surface_depth_format);
			assert(0);
			break;
		}
		}

		for (int i = 0; i < 4; ++i)
		{
			m_fbo.Renderbuffer(GL_COLOR_ATTACHMENT0 + i, m_rbo.GetId(i));
			checkForGlError(fmt::Format("m_fbo.Renderbuffer(GL_COLOR_ATTACHMENT%d)", i));
		}

		//m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT, m_rbo.GetId(4));
		//checkForGlError("m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT)");

		//if (m_surface_depth_format == 2)
		//{
		//	m_fbo.Renderbuffer(GL_STENCIL_ATTACHMENT, m_rbo.GetId(4));
		//	checkForGlError("m_fbo.Renderbuffer(GL_STENCIL_ATTACHMENT)");
		//}
	}

	if (!m_set_surface_clip_horizontal)
	{
		m_surface_clip_x = 0;
		m_surface_clip_w = RSXThread::m_width;
	}

	if (!m_set_surface_clip_vertical)
	{
		m_surface_clip_y = 0;
		m_surface_clip_h = RSXThread::m_height;
	}

	m_fbo.Bind();

	static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };

	switch (m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_NONE: break;

	case CELL_GCM_SURFACE_TARGET_0:
	{
		glDrawBuffer(draw_buffers[0]);
		checkForGlError("glDrawBuffer(0)");
		break;
	}
		
	case CELL_GCM_SURFACE_TARGET_1:
	{
		glDrawBuffer(draw_buffers[1]);
		checkForGlError("glDrawBuffer(1)");
		break;
	}
		
	case CELL_GCM_SURFACE_TARGET_MRT1:
	{
		glDrawBuffers(2, draw_buffers);
		checkForGlError("glDrawBuffers(2)");
		break;
	}
		
	case CELL_GCM_SURFACE_TARGET_MRT2:
	{
		glDrawBuffers(3, draw_buffers);
		checkForGlError("glDrawBuffers(3)");
		break;
	}

	case CELL_GCM_SURFACE_TARGET_MRT3:
	{
		glDrawBuffers(4, draw_buffers);
		checkForGlError("glDrawBuffers(4)");
		break;
	}

	default:
	{
		LOG_ERROR(RSX, "Bad surface color target: %d", m_surface_color_target);
		break;
	}
		
	}

	if (m_read_buffer)
	{
		u32 format = GL_BGRA;
		CellGcmDisplayInfo* buffers = vm::get_ptr<CellGcmDisplayInfo>(m_gcm_buffers_addr);
		u32 addr = GetAddress(buffers[m_gcm_current_buffer].offset, CELL_GCM_LOCATION_LOCAL);
		u32 width = buffers[m_gcm_current_buffer].width;
		u32 height = buffers[m_gcm_current_buffer].height;
		glDrawPixels(width, height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, vm::get_ptr(addr));
	}
	*/
}

bool GLGSRender::domethod(u32 cmd, u32 arg)
{
	if (cmd != NV4097_CLEAR_SURFACE)
		return false;

	InitDrawBuffers();

	/*
	if (m_set_color_mask)
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
	*/

	return true;

}

void GLGSRender::flip(int buffer)
{
}
