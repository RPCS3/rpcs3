#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/rPlatform.h" // only for rImage
#include "Utilities/File.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "GLGSRender.h"

#define CMD_DEBUG 0
#define DUMP_VERTEX_DATA 0

#if CMD_DEBUG
#define CMD_LOG(...) LOG_NOTICE(RSX, __VA_ARGS__)
#else
#define CMD_LOG(...)
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

		static const GLint swizzleMaskB8[] = { GL_BLUE, GL_BLUE, GL_BLUE, GL_BLUE };
		glRemap = swizzleMaskB8;
		break;
	}

	case CELL_GCM_TEXTURE_A1R5G5B5:
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);

		// TODO: texture swizzling
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, pixels);
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
		break;
	}

	case CELL_GCM_TEXTURE_A4R4G4B4:
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, pixels);

		// We read it in as R4G4B4A4, so we need to remap each component.
		static const GLint swizzleMaskA4R4G4B4[] = { GL_BLUE, GL_ALPHA, GL_RED, GL_GREEN };
		glRemap = swizzleMaskA4R4G4B4;
		break;
	}

	case CELL_GCM_TEXTURE_R5G6B5:
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex.width(), tex.height(), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
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

			log2width = (u32)log2(tex.width());
			log2height = (u32)log2(tex.height());

			for (int i = 0; i < tex.height(); i++)
			{
				for (int j = 0; j < tex.width(); j++)
				{
					dst[(i*tex.height()) + j] = src[rsx::linear_to_swizzle(j, i, 0, log2width, log2height, 0)];
				}
			}
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, is_swizzled ? unswizzledPixels : pixels);
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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RG, GL_UNSIGNED_BYTE, pixels);

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

		free(unswizzledPixels);
		break;
	}

	case CELL_GCM_TEXTURE_DEPTH24_D8: //  24-bit unsigned fixed-point number and 8 bits of garbage
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, pixels);
		break;
	}

	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: // 24-bit unsigned float and 8 bits of garbage
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, pixels);
		break;
	}

	case CELL_GCM_TEXTURE_DEPTH16: // 16-bit unsigned fixed-point number
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_SHORT, pixels);
		break;
	}

	case CELL_GCM_TEXTURE_DEPTH16_FLOAT: // 16-bit unsigned float
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, tex.width(), tex.height(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, pixels);
		break;
	}

	case CELL_GCM_TEXTURE_X16: // A 16-bit fixed-point number
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RED, GL_UNSIGNED_SHORT, pixels);
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

		static const GLint swizzleMaskX16[] = { GL_RED, GL_ONE, GL_RED, GL_ONE };
		glRemap = swizzleMaskX16;
		break;
	}

	case CELL_GCM_TEXTURE_Y16_X16: // Two 16-bit fixed-point numbers
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RG, GL_UNSIGNED_SHORT, pixels);
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
		static const GLint swizzleMaskX32_Y16_X16[] = { GL_GREEN, GL_RED, GL_GREEN, GL_RED };
		glRemap = swizzleMaskX32_Y16_X16;
		break;
	}

	case CELL_GCM_TEXTURE_R5G5B5A1:
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, pixels);
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
		break;
	}

	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: // Four fp16 values
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_HALF_FLOAT, pixels);
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
		break;
	}

	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: // Four fp32 values
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_FLOAT, pixels);
		break;
	}

	case CELL_GCM_TEXTURE_X32_FLOAT: // One 32-bit floating-point number
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RED, GL_FLOAT, pixels);

		static const GLint swizzleMaskX32_FLOAT[] = { GL_RED, GL_ONE, GL_ONE, GL_ONE };
		glRemap = swizzleMaskX32_FLOAT;
		break;
	}

	case CELL_GCM_TEXTURE_D1R5G5B5:
	{
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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, pixels);

		static const GLint swizzleMaskX32_D8R8G8B8[] = { GL_ONE, GL_RED, GL_GREEN, GL_BLUE };
		glRemap = swizzleMaskX32_D8R8G8B8;
		break;
	}


	case CELL_GCM_TEXTURE_Y16_X16_FLOAT: // Two fp16 values
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RG, GL_HALF_FLOAT, pixels);
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

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

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, gl_tex_zfunc[tex.zfunc()]);

	glTexEnvi(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, tex.bias());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, (tex.min_lod() >> 8));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, (tex.max_lod() >> 8));

	

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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_mag_filter[tex.mag_filter()]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, GetMaxAniso(tex.max_aniso()));

	

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

GLGSRender::GLGSRender()
{
	m_frame = GetGSFrame(GSFrameType::OpenGLFrame);
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
}

void GLGSRender::begin()
{
	rsx::thread::begin();

	if (!load_program())
	{
		//no program - no drawing
		return;
	}

	read_buffers();
}

void GLGSRender::end()
{
	write_buffers();

	rsx::thread::end();
}

void GLGSRender::oninit()
{
	m_draw_frames = 1;
	m_skip_frames = 0;

	m_frame->Show();
}

void GLGSRender::oninit_thread()
{
	m_context = m_frame->GetNewContext();
	m_frame->SetCurrent(m_context);

	gl::init();

	is_intel_vendor = strstr((const char*)glGetString(GL_VENDOR), "Intel");
	
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
}

void GLGSRender::onexit_thread()
{
	glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
}

void GLGSRender::onreset()
{
}

void nv4097_clear_surface(u32 arg, GLGSRender* renderer)
{
	u16 clear_x = rsx::method_registers[NV4097_SET_CLEAR_RECT_HORIZONTAL];
	u16 clear_y = rsx::method_registers[NV4097_SET_CLEAR_RECT_VERTICAL];
	u16 clear_w = rsx::method_registers[NV4097_SET_CLEAR_RECT_HORIZONTAL] >> 16;
	u16 clear_h = rsx::method_registers[NV4097_SET_CLEAR_RECT_VERTICAL] >> 16;

	if (clear_w || clear_h)
	{
		glViewport(clear_x, clear_y, clear_w, clear_h);
	}

	GLbitfield mask = 0;

	if (arg & 0x1)
	{
		u32 surface_depth_format = (rsx::method_registers[NV4097_SET_SURFACE_FORMAT] >> 5) & 0x7;
		u32 max_depth_value = surface_depth_format == CELL_GCM_SURFACE_Z16 ? 0x0000ffff : 0x00ffffff;

		u32 clear_depth = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] >> 8;

		glDepthMask(GL_TRUE);
		glClearDepth(double(clear_depth) / max_depth_value);

		mask |= GL_DEPTH_BUFFER_BIT;
	}

	if (arg & 0x2)
	{
		u8 clear_stencil = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] & 0xff;

		glStencilMask(0xff);
		glClearStencil(clear_stencil);

		mask |= GL_STENCIL_BUFFER_BIT;
	}

	if (arg & 0xf0)
	{
		u32 clear_color = rsx::method_registers[NV4097_SET_COLOR_CLEAR_VALUE];
		u8 clear_a = clear_color >> 24;
		u8 clear_r = clear_color >> 16;
		u8 clear_g = clear_color >> 8;
		u8 clear_b = clear_color;

		glColorMask(arg & 0x20, arg & 0x40, arg & 0x80, arg & 0x10);
		glClearColor(clear_r / 255.f, clear_g / 255.f, clear_b / 255.f, clear_a / 255.f);

		mask |= GL_COLOR_BUFFER_BIT;
	}

	if (mask)
	{
		renderer->read_buffers();
		glClear(mask);
		renderer->write_buffers();
	}
}

using rsx_method_impl_t = void(*)(u32, GLGSRender*);

static const std::unordered_map<u32, rsx_method_impl_t> g_gl_method_tbl =
{
	{ NV4097_CLEAR_SURFACE, nv4097_clear_surface },
};

bool GLGSRender::domethod(u32 cmd, u32 arg)
{
	auto found = g_gl_method_tbl.find(cmd);

	if (found == g_gl_method_tbl.end())
	{
		return false;
	}

	found->second(arg, this);
	return true;
}

void GLGSRender::load_vertex_data()
{

}

void GLGSRender::load_fragment_data()
{

}

void GLGSRender::load_indexes()
{

}

bool GLGSRender::load_program()
{
	return false;
}

struct color_swizzle
{
	gl::texture::channel a = gl::texture::channel::a;
	gl::texture::channel r = gl::texture::channel::r;
	gl::texture::channel g = gl::texture::channel::g;
	gl::texture::channel b = gl::texture::channel::b;

	color_swizzle() = default;
	color_swizzle(gl::texture::channel a, gl::texture::channel r, gl::texture::channel g, gl::texture::channel b)
		: a(a), r(r), g(g), b(b)
	{
	}
};

struct color_format
{
	gl::texture::type type;
	gl::texture::format format;
	bool swap_bytes;
	int channel_count;
	int channel_size;
	color_swizzle swizzle;
};

color_format surface_color_format_to_gl(int color_format)
{
	//color format
	switch (color_format)
	{
	case CELL_GCM_SURFACE_R5G6B5:
		return{ gl::texture::type::ushort_5_6_5, gl::texture::format::bgr, false, 3, 2 };

	case CELL_GCM_SURFACE_A8R8G8B8:
		return{ gl::texture::type::uint_8_8_8_8, gl::texture::format::bgra, false, 4, 1 };

	case CELL_GCM_SURFACE_X8R8G8B8_O8R8G8B8:
		return{ gl::texture::type::uint_8_8_8_8, gl::texture::format::bgra, false, 4, 1,
		{ gl::texture::channel::one, gl::texture::channel::r, gl::texture::channel::g, gl::texture::channel::b } };

	case CELL_GCM_SURFACE_F_W16Z16Y16X16:
		return{ gl::texture::type::f16, gl::texture::format::rgba, true, 4, 2 };

	case CELL_GCM_SURFACE_F_W32Z32Y32X32:
		return{ gl::texture::type::f32, gl::texture::format::rgba, true, 4, 4 };

	case CELL_GCM_SURFACE_B8:
	case CELL_GCM_SURFACE_X1R5G5B5_Z1R5G5B5:
	case CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5:
	case CELL_GCM_SURFACE_X8R8G8B8_Z8R8G8B8:
	case CELL_GCM_SURFACE_G8B8:
	case CELL_GCM_SURFACE_F_X32:
	case CELL_GCM_SURFACE_X8B8G8R8_Z8B8G8R8:
	case CELL_GCM_SURFACE_X8B8G8R8_O8B8G8R8:
	case CELL_GCM_SURFACE_A8B8G8R8:
	default:
		LOG_ERROR(RSX, "Surface color buffer: Unsupported surface color format (0x%x)", color_format);
		return{ gl::texture::type::uint_8_8_8_8, gl::texture::format::bgra, false, 4, 1 };
	}
}

std::pair<gl::texture::type, gl::texture::format> surface_depth_format_to_gl(int depth_format)
{
	switch (depth_format)
	{
	case CELL_GCM_SURFACE_Z16:
		return std::make_pair(gl::texture::type::ushort, gl::texture::format::depth);

	default:
		LOG_ERROR(RSX, "Surface depth buffer: Unsupported surface depth format (0x%x)", depth_format);
	case CELL_GCM_SURFACE_Z24S8:
		return std::make_pair(gl::texture::type::uint_24_8, gl::texture::format::depth_stencil);
		//return std::make_pair(gl::texture::type::f32, gl::texture::format::depth);
	}
}

void GLGSRender::init_buffers()
{
	u32 surface_format = rsx::method_registers[NV4097_SET_SURFACE_FORMAT];

	/*
	u32 clip_vertical = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL];
	u32 clip_horizontal = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL];

	u32 clip_width = clip_horizontal >> 16;
	u32 clip_height = clip_vertical >> 16;
	u32 clip_x = clip_vertical;
	u32 clip_y = clip_horizontal;
	*/

	if (!m_draw_fbo || m_surface.format != surface_format)
	{
		m_surface.unpack(surface_format);
		m_draw_fbo.recreate();
		m_draw_tex_depth_stencil.recreate(gl::texture::target::texture2D);

		auto format = surface_color_format_to_gl(m_surface.color_format);

		for (int i = 0; i < rsx::limits::color_buffers_count; ++i)
		{
			m_draw_tex_color[i].recreate(gl::texture::target::texture2D);
			m_draw_tex_color[i].config()
				.size(m_surface.width, m_surface.height)
				.type(format.type)
				.format(format.format)
				.swizzle(format.swizzle.r, format.swizzle.g, format.swizzle.b, format.swizzle.a);

			m_draw_tex_color[i].pixel_pack_settings()
				.swap_bytes(format.swap_bytes);

			m_draw_fbo.color[i] = m_draw_tex_color[i];
		}

		switch (m_surface.depth_format)
		{
		case CELL_GCM_SURFACE_Z16:
		{
			m_draw_tex_depth_stencil.config()
				.size(m_surface.width, m_surface.height)
				.type(gl::texture::type::ushort)
				.format(gl::texture::format::depth)
				.internal_format(gl::texture::format::depth16);

			m_draw_fbo.depth = m_draw_tex_depth_stencil;
			break;
		}

		case CELL_GCM_SURFACE_Z24S8:
		{
			m_draw_tex_depth_stencil.config()
				.size(m_surface.width, m_surface.height)
				.type(gl::texture::type::uint_24_8)
				.format(gl::texture::format::depth_stencil)
				.internal_format(gl::texture::format::depth24_stencil8);

			m_draw_fbo.depth_stencil = m_draw_tex_depth_stencil;
			break;
		}

		case 0:
			break;

		default:
		{
			LOG_ERROR(RSX, "Bad depth format! (%d)", m_surface.depth_format);
			assert(0);
			break;
		}
		}

		m_draw_tex_depth_stencil.pixel_pack_settings().aligment(1);
		m_draw_tex_depth_stencil.pixel_unpack_settings().aligment(1);
	}
}

static const u32 mr_color_offset[rsx::limits::color_buffers_count] =
{
	NV4097_SET_SURFACE_COLOR_AOFFSET,
	NV4097_SET_SURFACE_COLOR_BOFFSET,
	NV4097_SET_SURFACE_COLOR_COFFSET,
	NV4097_SET_SURFACE_COLOR_DOFFSET
};

static const u32 mr_color_dma[rsx::limits::color_buffers_count] =
{
	NV4097_SET_CONTEXT_DMA_COLOR_A,
	NV4097_SET_CONTEXT_DMA_COLOR_B,
	NV4097_SET_CONTEXT_DMA_COLOR_C,
	NV4097_SET_CONTEXT_DMA_COLOR_D
};

void GLGSRender::read_buffers()
{
	if (!m_draw_fbo)
		return;

	auto color_format = surface_color_format_to_gl(m_surface.color_format);

	auto read_color_buffers = [&](int index, int count)
	{
		for (int i = index; i < index + count; ++i)
		{
			u32 color_address = rsx::get_address(rsx::method_registers[mr_color_offset[i]], rsx::method_registers[mr_color_dma[i]]);
			m_draw_tex_color[i].copy_from(vm::get_ptr(color_address), color_format.format, color_format.type);
		}
	};

	switch (rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET])
	{
	case CELL_GCM_SURFACE_TARGET_NONE:
		break;

	case CELL_GCM_SURFACE_TARGET_0:
		read_color_buffers(0, 1);
		break;

	case CELL_GCM_SURFACE_TARGET_1:
		read_color_buffers(1, 1);
		break;

	case CELL_GCM_SURFACE_TARGET_MRT1:
		read_color_buffers(0, 2);
		break;

	case CELL_GCM_SURFACE_TARGET_MRT2:
		read_color_buffers(0, 3);
		break;

	case CELL_GCM_SURFACE_TARGET_MRT3:
		read_color_buffers(0, 4);
		break;
	}

	auto depth_format = surface_depth_format_to_gl(m_surface.depth_format);

	int pixel_size = m_surface.depth_format == CELL_GCM_SURFACE_Z16 ? 2 : 4;

	gl::buffer pbo_depth;

	pbo_depth.create(m_surface.width * m_surface.height * pixel_size);
	pbo_depth.map([&](GLubyte* pixels)
	{
		u32 depth_address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET], rsx::method_registers[NV4097_SET_CONTEXT_DMA_ZETA]);

		if (m_surface.depth_format == CELL_GCM_SURFACE_Z16)
		{
			u16 *dst = (u16*)pixels;
			const be_t<u16>* src = vm::get_ptr<const be_t<u16>>(depth_address);
			for (int i = 0, end = m_draw_tex_depth_stencil.width() * m_draw_tex_depth_stencil.height(); i < end; ++i)
			{
				dst[i] = src[i];
			}
		}
		else
		{
			u32 *dst = (u32*)pixels;
			const be_t<u32>* src = vm::get_ptr<const be_t<u32>>(depth_address);
			for (int i = 0, end = m_draw_tex_depth_stencil.width() * m_draw_tex_depth_stencil.height(); i < end; ++i)
			{
				dst[i] = src[i];
			}
		}
	}, gl::buffer::access::write);

	m_draw_tex_depth_stencil.copy_from(pbo_depth, depth_format.second, depth_format.first);

	//m_texture_depth.copy_from(vm::get_ptr(GetAddress(m_surface_offset_z, m_context_dma_z)),
	//	depth_format.second, depth_format.first);
}

void GLGSRender::write_buffers()
{
	if (!m_draw_fbo)
		return;

	auto color_format = surface_color_format_to_gl(m_surface.color_format);

	auto write_color_buffers = [&](int index, int count)
	{
		for (int i = index; i < index + count; ++i)
		{
			//TODO: swizzle
			u32 color_address = rsx::get_address(rsx::method_registers[mr_color_offset[i]], rsx::method_registers[mr_color_dma[i]]);
			m_draw_tex_color[i].copy_to(vm::get_ptr(color_address), color_format.format, color_format.type);
		}
	};

	switch (rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET])
	{
	case CELL_GCM_SURFACE_TARGET_NONE:
		break;

	case CELL_GCM_SURFACE_TARGET_0:
		write_color_buffers(0, 1);
		break;

	case CELL_GCM_SURFACE_TARGET_1:
		write_color_buffers(1, 1);
		break;

	case CELL_GCM_SURFACE_TARGET_MRT1:
		write_color_buffers(0, 2);
		break;

	case CELL_GCM_SURFACE_TARGET_MRT2:
		write_color_buffers(0, 3);
		break;

	case CELL_GCM_SURFACE_TARGET_MRT3:
		write_color_buffers(0, 4);
		break;
	}

	auto depth_format = surface_depth_format_to_gl(m_surface.depth_format);

	gl::buffer pbo_depth;

	int pixel_size = m_surface.depth_format == CELL_GCM_SURFACE_Z16 ? 2 : 4;
	pbo_depth.create(m_surface.width * m_surface.height * pixel_size);

	//m_texture_depth.copy_to(vm::get_ptr(GetAddress(m_surface_offset_z, m_context_dma_z)), depth_format.second, depth_format.first);
	m_draw_tex_depth_stencil.copy_to(pbo_depth, depth_format.second, depth_format.first);

	pbo_depth.map([&](GLubyte* pixels)
	{
		u32 depth_address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET], rsx::method_registers[NV4097_SET_CONTEXT_DMA_ZETA]);

		if (m_surface.depth_format == CELL_GCM_SURFACE_Z16)
		{
			const u16 *src = (const u16*)pixels;
			be_t<u16>* dst = vm::get_ptr<be_t<u16>>(depth_address);
			for (int i = 0, end = m_draw_tex_depth_stencil.width() * m_draw_tex_depth_stencil.height(); i < end; ++i)
			{
				dst[i] = src[i];
			}
		}
		else
		{
			const u32 *src = (const u32*)pixels;
			be_t<u32>* dst = vm::get_ptr<be_t<u32>>(depth_address);
			for (int i = 0, end = m_draw_tex_depth_stencil.width() * m_draw_tex_depth_stencil.height(); i < end; ++i)
			{
				dst[i] = src[i];
			}
		}

	}, gl::buffer::access::read);
}

void GLGSRender::flip(int buffer)
{
	u32 buffer_width = gcm_buffers[buffer].width;
	u32 buffer_height = gcm_buffers[buffer].height;
	u32 buffer_pitch = gcm_buffers[buffer].pitch;

	if (!m_flip_tex_color || m_flip_tex_color.width() != buffer_width || m_flip_tex_color.height() != buffer_height)
	{
		m_flip_tex_color.recreate(gl::texture::target::texture2D);

		glcheck(m_flip_tex_color.config()
			.size(buffer_width, buffer_height)
			.type(gl::texture::type::uint_8_8_8_8)
			.format(gl::texture::format::bgra));

		glcheck(m_flip_fbo.recreate());
		glcheck(m_flip_fbo.color = m_flip_tex_color);
	}

	glcheck(m_flip_fbo.draw(m_flip_fbo.color));

	glcheck(m_flip_tex_color.copy_from(
		vm::get_ptr(rsx::get_address(gcm_buffers[buffer].offset, CELL_GCM_LOCATION_LOCAL)),
		gl::texture::format::bgra, gl::texture::type::uint_8_8_8_8, gl::pixel_unpack_settings().row_length(buffer_pitch / 4)));

	area screen_area = coordi({}, { buffer_width, buffer_height });

	coordi aspect_ratio;
	if (1) //enable aspect ratio
	{
		sizei csize = m_frame->GetClientSize();
		sizei new_size = csize;

		const double aq = (double)buffer_width / buffer_height;
		const double rq = (double)new_size.width / new_size.height;
		const double q = aq / rq;

		if (q > 1.0)
		{
			new_size.height = int(new_size.height / q);
			aspect_ratio.y = (csize.height - new_size.height) / 2;
		}
		else if (q < 1.0)
		{
			new_size.width = int(new_size.width * q);
			aspect_ratio.x = (csize.width - new_size.width) / 2;
		}

		aspect_ratio.size = new_size;
	}
	else
	{
		aspect_ratio.size = m_frame->GetClientSize();
	}

	glcheck(m_flip_fbo.blit(gl::screen, screen_area, area(aspect_ratio).flipped_vertical()));

	m_frame->Flip(m_context);
}
