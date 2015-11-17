#include "stdafx.h"
#include "Utilities/rPlatform.h" // only for rImage
#include "Utilities/File.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/state.h"
#include "GLGSRender.h"

#define DUMP_VERTEX_DATA 0

void GLTexture::create()
{
	if (m_id)
	{
		remove();
	}

	glGenTextures(1, &m_id);
	bind();
}

int GLTexture::gl_wrap(int wrap)
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

float GLTexture::max_aniso(int aniso)
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

void GLTexture::init(rsx::texture& tex)
{
	if (!m_id)
		create();

	bind();

	const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
	//LOG_WARNING(RSX, "texture addr = 0x%x, width = %d, height = %d, max_aniso=%d, mipmap=%d, remap=0x%x, zfunc=0x%x, wraps=0x%x, wrapt=0x%x, wrapr=0x%x, minlod=0x%x, maxlod=0x%x", 
	//	m_offset, m_width, m_height, m_maxaniso, m_mipmap, m_remap, m_zfunc, m_wraps, m_wrapt, m_wrapr, m_minlod, m_maxlod);
	
	//TODO: safe init

	int format = tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
	bool is_swizzled = !(tex.format() & CELL_GCM_TEXTURE_LN);

	const u8* pixels = vm::ps3::_ptr<u8>(texaddr);
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
			unswizzledPixels[i * 4 + 0] = convert_6_to_8((c >> 10) & 0x3F);
			unswizzledPixels[i * 4 + 1] = convert_5_to_8((c >> 5) & 0x1F);
			unswizzledPixels[i * 4 + 2] = convert_5_to_8((c >> 0) & 0x1F);
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

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_wrap(tex.wrap_s()));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_wrap(tex.wrap_t()));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, gl_wrap(tex.wrap_r()));

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
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso(tex.max_aniso()));

	//Unbind();

	if (is_swizzled && format == CELL_GCM_TEXTURE_A8R8G8B8)
	{
		free(unswizzledPixels);
	}
}

void GLTexture::save(rsx::texture& tex, const std::string& name)
{
	if (!m_id || !tex.offset() || !tex.width() || !tex.height()) return;

	const u32 texPixelCount = tex.width() * tex.height();

	u32* alldata = new u32[texPixelCount];

	bind();

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

	fs::file(name + ".raw", fom::rewrite).write(alldata, texPixelCount * 4);

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

void GLTexture::save(rsx::texture& tex)
{
	static const std::string& dir_path = "textures";
	static const std::string& file_fmt = dir_path + "/" + "tex[%d].png";

	if (!fs::exists(dir_path)) fs::create_dir(dir_path);

	u32 count = 0;
	while (fs::exists(fmt::format(file_fmt.c_str(), count))) count++;
	save(tex, fmt::format(file_fmt.c_str(), count));
}

void GLTexture::bind()
{
	glBindTexture(GL_TEXTURE_2D, m_id);
}

void GLTexture::unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GLTexture::remove()
{
	if (m_id)
	{
		glDeleteTextures(1, &m_id);
		m_id = 0;
	}
}

u32 GLTexture::id() const
{
	return m_id;
}

GLGSRender::GLGSRender() : GSRender(frame_type::OpenGL)
{
}

u32 GLGSRender::enable(u32 condition, u32 cap)
{
	if (condition)
	{
		glEnable(cap);
	}
	else
	{
		glDisable(cap);
	}

	return condition;
}

u32 GLGSRender::enable(u32 condition, u32 cap, u32 index)
{
	if (condition)
	{
		glEnablei(cap, index);
	}
	else
	{
		glDisablei(cap, index);
	}

	return condition;
}

extern CellGcmContextData current_context;

void GLGSRender::begin()
{
	rsx::thread::begin();

	if (!load_program())
	{
		//no program - no drawing
		return;
	}

	init_buffers();

	u32 color_mask = rsx::method_registers[NV4097_SET_COLOR_MASK];
	bool color_mask_b = color_mask & 0xff;
	bool color_mask_g = (color_mask >> 8) & 0xff;
	bool color_mask_r = (color_mask >> 16) & 0xff;
	bool color_mask_a = (color_mask >> 24) & 0xff;

	__glcheck glColorMask(color_mask_r, color_mask_g, color_mask_b, color_mask_a);
	__glcheck glDepthMask(rsx::method_registers[NV4097_SET_DEPTH_MASK]);
	__glcheck glStencilMask(rsx::method_registers[NV4097_SET_STENCIL_MASK]);

	//int viewport_x = int(rsx::method_registers[NV4097_SET_VIEWPORT_HORIZONTAL] & 0xffff);
	//int viewport_y = int(rsx::method_registers[NV4097_SET_VIEWPORT_VERTICAL] & 0xffff);
	//int viewport_w = int(rsx::method_registers[NV4097_SET_VIEWPORT_HORIZONTAL] >> 16);
	//int viewport_h = int(rsx::method_registers[NV4097_SET_VIEWPORT_VERTICAL] >> 16);
	//glViewport(viewport_x, viewport_y, viewport_w, viewport_h);

	//scissor test is always enabled
	glEnable(GL_SCISSOR_TEST);

	u32 scissor_horizontal = rsx::method_registers[NV4097_SET_SCISSOR_HORIZONTAL];
	u32 scissor_vertical = rsx::method_registers[NV4097_SET_SCISSOR_VERTICAL];
	u16 scissor_x = scissor_horizontal;
	u16 scissor_w = scissor_horizontal >> 16;
	u16 scissor_y = scissor_vertical;
	u16 scissor_h = scissor_vertical >> 16;

	__glcheck glScissor(scissor_x, scissor_y, scissor_w, scissor_h);

	if (__glcheck enable(rsx::method_registers[NV4097_SET_DEPTH_TEST_ENABLE], GL_DEPTH_TEST))
	{
		__glcheck glDepthFunc(rsx::method_registers[NV4097_SET_DEPTH_FUNC]);
		__glcheck glDepthMask(rsx::method_registers[NV4097_SET_DEPTH_MASK]);
	}

	if (glDepthBoundsEXT && (__glcheck enable(rsx::method_registers[NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE], GL_DEPTH_BOUNDS_TEST_EXT)))
	{
		__glcheck glDepthBoundsEXT((f32&)rsx::method_registers[NV4097_SET_DEPTH_BOUNDS_MIN], (f32&)rsx::method_registers[NV4097_SET_DEPTH_BOUNDS_MAX]);
	}

	__glcheck glDepthRange((f32&)rsx::method_registers[NV4097_SET_CLIP_MIN], (f32&)rsx::method_registers[NV4097_SET_CLIP_MAX]);
	__glcheck enable(rsx::method_registers[NV4097_SET_DITHER_ENABLE], GL_DITHER);

	if (__glcheck enable(rsx::method_registers[NV4097_SET_ALPHA_TEST_ENABLE], GL_ALPHA_TEST))
	{
		//TODO: NV4097_SET_ALPHA_REF must be converted to f32
		//glcheck(glAlphaFunc(rsx::method_registers[NV4097_SET_ALPHA_FUNC], rsx::method_registers[NV4097_SET_ALPHA_REF]));
	}

	if (__glcheck enable(rsx::method_registers[NV4097_SET_BLEND_ENABLE], GL_BLEND))
	{
		u32 sfactor = rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR];
		u32 dfactor = rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR];
		u16 sfactor_rgb = sfactor;
		u16 sfactor_a = sfactor >> 16;
		u16 dfactor_rgb = dfactor;
		u16 dfactor_a = dfactor >> 16;

		__glcheck glBlendFuncSeparate(sfactor_rgb, dfactor_rgb, sfactor_a, dfactor_a);

		if (m_surface.color_format == CELL_GCM_SURFACE_F_W16Z16Y16X16) //TODO: check another color formats
		{
			u32 blend_color = rsx::method_registers[NV4097_SET_BLEND_COLOR];
			u32 blend_color2 = rsx::method_registers[NV4097_SET_BLEND_COLOR2];

			u16 blend_color_r = blend_color;
			u16 blend_color_g = blend_color >> 16;
			u16 blend_color_b = blend_color2;
			u16 blend_color_a = blend_color2 >> 16;

			__glcheck glBlendColor(blend_color_r / 65535.f, blend_color_g / 65535.f, blend_color_b / 65535.f, blend_color_a / 65535.f);
		}
		else
		{
			u32 blend_color = rsx::method_registers[NV4097_SET_BLEND_COLOR];
			u8 blend_color_r = blend_color;
			u8 blend_color_g = blend_color >> 8;
			u8 blend_color_b = blend_color >> 16;
			u8 blend_color_a = blend_color >> 24;

			__glcheck glBlendColor(blend_color_r / 255.f, blend_color_g / 255.f, blend_color_b / 255.f, blend_color_a / 255.f);
		}

		u32 equation = rsx::method_registers[NV4097_SET_BLEND_EQUATION];
		u16 equation_rgb = equation;
		u16 equation_a = equation >> 16;

		__glcheck glBlendEquationSeparate(equation_rgb, equation_a);
	}
	
	if (__glcheck enable(rsx::method_registers[NV4097_SET_STENCIL_TEST_ENABLE], GL_STENCIL_TEST))
	{
		__glcheck glStencilFunc(rsx::method_registers[NV4097_SET_STENCIL_FUNC], rsx::method_registers[NV4097_SET_STENCIL_FUNC_REF],
			rsx::method_registers[NV4097_SET_STENCIL_FUNC_MASK]);
		__glcheck glStencilOp(rsx::method_registers[NV4097_SET_STENCIL_OP_FAIL], rsx::method_registers[NV4097_SET_STENCIL_OP_ZFAIL],
			rsx::method_registers[NV4097_SET_STENCIL_OP_ZPASS]);
	}

	if (__glcheck enable(rsx::method_registers[NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE], GL_STENCIL_TEST_TWO_SIDE_EXT))
	{
		__glcheck glStencilMaskSeparate(GL_BACK, rsx::method_registers[NV4097_SET_BACK_STENCIL_MASK]);
		__glcheck glStencilFuncSeparate(GL_BACK, rsx::method_registers[NV4097_SET_BACK_STENCIL_FUNC],
			rsx::method_registers[NV4097_SET_BACK_STENCIL_FUNC_REF], rsx::method_registers[NV4097_SET_BACK_STENCIL_FUNC_MASK]);
		__glcheck glStencilOpSeparate(GL_BACK, rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_FAIL],
			rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_ZFAIL], rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_ZPASS]);
	}

	__glcheck glShadeModel(rsx::method_registers[NV4097_SET_SHADE_MODE]);

	if (u32 blend_mrt = rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT])
	{
		__glcheck enable(blend_mrt & 2, GL_BLEND, GL_COLOR_ATTACHMENT1);
		__glcheck enable(blend_mrt & 4, GL_BLEND, GL_COLOR_ATTACHMENT2);
		__glcheck enable(blend_mrt & 8, GL_BLEND, GL_COLOR_ATTACHMENT3);
	}
	
	if (__glcheck enable(rsx::method_registers[NV4097_SET_LOGIC_OP_ENABLE], GL_LOGIC_OP))
	{
		__glcheck glLogicOp(rsx::method_registers[NV4097_SET_LOGIC_OP]);
	}

	u32 line_width = rsx::method_registers[NV4097_SET_LINE_WIDTH];
	__glcheck glLineWidth((line_width >> 3) + (line_width & 7) / 8.f);
	__glcheck enable(rsx::method_registers[NV4097_SET_LINE_SMOOTH_ENABLE], GL_LINE_SMOOTH);

	//TODO
	//NV4097_SET_ANISO_SPREAD

	//TODO
	/*
	glcheck(glFogi(GL_FOG_MODE, rsx::method_registers[NV4097_SET_FOG_MODE]));
	f32 fog_p0 = (f32&)rsx::method_registers[NV4097_SET_FOG_PARAMS + 0];
	f32 fog_p1 = (f32&)rsx::method_registers[NV4097_SET_FOG_PARAMS + 1];

	f32 fog_start = (2 * fog_p0 - (fog_p0 - 2) / fog_p1) / (fog_p0 - 1);
	f32 fog_end = (2 * fog_p0 - 1 / fog_p1) / (fog_p0 - 1);

	glFogf(GL_FOG_START, fog_start);
	glFogf(GL_FOG_END, fog_end);
	*/
	//NV4097_SET_FOG_PARAMS

	__glcheck enable(rsx::method_registers[NV4097_SET_POLY_OFFSET_POINT_ENABLE], GL_POLYGON_OFFSET_POINT);
	__glcheck enable(rsx::method_registers[NV4097_SET_POLY_OFFSET_LINE_ENABLE], GL_POLYGON_OFFSET_LINE);
	__glcheck enable(rsx::method_registers[NV4097_SET_POLY_OFFSET_FILL_ENABLE], GL_POLYGON_OFFSET_FILL);

	__glcheck glPolygonOffset((f32&)rsx::method_registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR],
		(f32&)rsx::method_registers[NV4097_SET_POLYGON_OFFSET_BIAS]);

	//NV4097_SET_SPECULAR_ENABLE
	//NV4097_SET_TWO_SIDE_LIGHT_EN
	//NV4097_SET_FLAT_SHADE_OP
	//NV4097_SET_EDGE_FLAG

	u32 clip_plane_control = rsx::method_registers[NV4097_SET_USER_CLIP_PLANE_CONTROL];
	u8 clip_plane_0 = clip_plane_control & 0xf;
	u8 clip_plane_1 = (clip_plane_control >> 4) & 0xf;
	u8 clip_plane_2 = (clip_plane_control >> 8) & 0xf;
	u8 clip_plane_3 = (clip_plane_control >> 12) & 0xf;
	u8 clip_plane_4 = (clip_plane_control >> 16) & 0xf;
	u8 clip_plane_5 = (clip_plane_control >> 20) & 0xf;

	//TODO
	if (__glcheck enable(clip_plane_0, GL_CLIP_DISTANCE0)) {}
	if (__glcheck enable(clip_plane_1, GL_CLIP_DISTANCE1)) {}
	if (__glcheck enable(clip_plane_2, GL_CLIP_DISTANCE2)) {}
	if (__glcheck enable(clip_plane_3, GL_CLIP_DISTANCE3)) {}
	if (__glcheck enable(clip_plane_4, GL_CLIP_DISTANCE4)) {}
	if (__glcheck enable(clip_plane_5, GL_CLIP_DISTANCE5)) {}

	__glcheck enable(rsx::method_registers[NV4097_SET_POLY_OFFSET_FILL_ENABLE], GL_POLYGON_OFFSET_FILL);

	if (__glcheck enable(rsx::method_registers[NV4097_SET_POLYGON_STIPPLE], GL_POLYGON_STIPPLE))
	{
		__glcheck glPolygonStipple((GLubyte*)(rsx::method_registers + NV4097_SET_POLYGON_STIPPLE_PATTERN));
	}

	__glcheck glPolygonMode(GL_FRONT, rsx::method_registers[NV4097_SET_FRONT_POLYGON_MODE]);
	__glcheck glPolygonMode(GL_BACK, rsx::method_registers[NV4097_SET_BACK_POLYGON_MODE]);

	if (__glcheck enable(rsx::method_registers[NV4097_SET_CULL_FACE_ENABLE], GL_CULL_FACE))
	{
		__glcheck glCullFace(rsx::method_registers[NV4097_SET_CULL_FACE]);
		__glcheck glFrontFace(rsx::method_registers[NV4097_SET_FRONT_FACE]);
	}

	__glcheck enable(rsx::method_registers[NV4097_SET_POLY_SMOOTH_ENABLE], GL_POLYGON_SMOOTH);

	//NV4097_SET_COLOR_KEY_COLOR
	//NV4097_SET_SHADER_CONTROL
	//NV4097_SET_ZMIN_MAX_CONTROL
	//NV4097_SET_ANTI_ALIASING_CONTROL
	//NV4097_SET_CLIP_ID_TEST_ENABLE

	if (__glcheck enable(rsx::method_registers[NV4097_SET_RESTART_INDEX_ENABLE], GL_PRIMITIVE_RESTART))
	{
		__glcheck glPrimitiveRestartIndex(rsx::method_registers[NV4097_SET_RESTART_INDEX]);
	}

	if (__glcheck enable(rsx::method_registers[NV4097_SET_LINE_STIPPLE], GL_LINE_STIPPLE))
	{
		u32 line_stipple_pattern = rsx::method_registers[NV4097_SET_LINE_STIPPLE_PATTERN];
		u16 factor = line_stipple_pattern;
		u16 pattern = line_stipple_pattern >> 16;
		__glcheck glLineStipple(factor, pattern);
	}
}

template<typename T, int count>
struct apply_attrib_t;

template<typename T>
struct apply_attrib_t<T, 1>
{
	static void func(gl::glsl::program& program, int location, const T* data)
	{
		program.attribs[location] = data[0];
	}
};

template<typename T>
struct apply_attrib_t<T, 2>
{
	static void func(gl::glsl::program& program, int location, const T* data)
	{
		program.attribs[location] = color2_base<T>{ data[0], data[1] };
	}
};

template<typename T>
struct apply_attrib_t<T, 3>
{
	static void func(gl::glsl::program& program, int location, const T* data)
	{
		program.attribs[location] = color3_base<T>{ data[0], data[1], data[2] };
	}
};
template<typename T>
struct apply_attrib_t<T, 4>
{
	static void func(gl::glsl::program& program, int location, const T* data)
	{
		program.attribs[location] = color4_base<T>{ data[0], data[1], data[2], data[3] };
	}
};


template<typename T, int count>
void apply_attrib_array(gl::glsl::program& program, int location, const std::vector<u8>& data)
{
	for (size_t offset = 0; offset < data.size(); offset += count * sizeof(T))
	{
		apply_attrib_t<T, count>::func(program, location, (T*)(data.data() + offset));
	}
}

void GLGSRender::end()
{
	if (!draw_fbo || !vertex_draw_count)
	{
		rsx::thread::end();
		return;
	}

	//LOG_NOTICE(Log::RSX, "draw()");

	draw_fbo.bind();
	m_program->use();

	//setup textures
	for (int i = 0; i < rsx::limits::textures_count; ++i)
	{
		if (!textures[i].enabled())
			continue;

		int location;
		if (m_program->uniforms.has_location("tex" + std::to_string(i), &location))
		{
			__glcheck m_gl_textures[i].init(textures[i]);
			__glcheck m_program->uniforms.texture(location, i, gl::texture_view(gl::texture::target::texture2D, m_gl_textures[i].id()));
		}
	}

	//initialize vertex attributes
	static const gl::buffer_pointer::type gl_types[] =
	{
		gl::buffer_pointer::type::f32,

		gl::buffer_pointer::type::s16,
		gl::buffer_pointer::type::f32,
		gl::buffer_pointer::type::f16,
		gl::buffer_pointer::type::u8,
		gl::buffer_pointer::type::s16,
		gl::buffer_pointer::type::f32, // Needs conversion
		gl::buffer_pointer::type::u8
	};

	static const bool gl_normalized[] =
	{
		false,

		true,
		false,
		false,
		true,
		false,
		true,
		false
	};

	//merge all vertex arrays
	std::vector<u8> vertex_arrays_data;
	size_t vertex_arrays_offsets[rsx::limits::vertex_count];

#if	DUMP_VERTEX_DATA
	fs::file dump("VertexDataArray.dump", o_create | o_write);
	Emu.Pause();
#endif

	for (int index = 0; index < rsx::limits::vertex_count; ++index)
	{
		size_t position = vertex_arrays_data.size();
		vertex_arrays_offsets[index] = position;

		if (vertex_arrays[index].empty())
			continue;

		size_t size = vertex_arrays[index].size();
		vertex_arrays_data.resize(position + size);

		memcpy(vertex_arrays_data.data() + position, vertex_arrays[index].data(), size);

#if	DUMP_VERTEX_DATA
		auto &vertex_info = vertex_arrays_info[index];
		dump.write(fmt::format("VertexData[%d]:\n", index));
		switch (vertex_info.type)
		{
		case CELL_GCM_VERTEX_S1:
			for (u32 j = 0; j < vertex_arrays[index].size(); j += 2)
			{
				dump.write(fmt::format("%d\n", *(u16*)&vertex_arrays[index][j]));
				if (!(((j + 2) / 2) % vertex_info.size)) dump.write("\n");
			}
			break;

		case CELL_GCM_VERTEX_F:
			for (u32 j = 0; j < vertex_arrays[index].size(); j += 4)
			{
				dump.write(fmt::format("%.01f\n", *(float*)&vertex_arrays[index][j]));
				if (!(((j + 4) / 4) % vertex_info.size)) dump.write("\n");
			}
			break;

		case CELL_GCM_VERTEX_SF:
			for (u32 j = 0; j < vertex_arrays[index].size(); j += 2)
			{
				dump.write(fmt::format("%.01f\n", *(float*)&vertex_arrays[index][j]));
				if (!(((j + 2) / 2) % vertex_info.size)) dump.write("\n");
			}
			break;

		case CELL_GCM_VERTEX_UB:
			for (u32 j = 0; j < vertex_arrays[index].size(); ++j)
			{
				dump.write(fmt::format("%d\n", vertex_arrays[index][j]));
				if (!((j + 1) % vertex_info.size)) dump.write("\n");
			}
			break;

		case CELL_GCM_VERTEX_S32K:
			for (u32 j = 0; j < vertex_arrays[index].size(); j += 2)
			{
				dump.write(fmt::format("%d\n", *(u16*)&vertex_arrays[index][j]));
				if (!(((j + 2) / 2) % vertex_info.size)) dump.write("\n");
			}
			break;

			// case CELL_GCM_VERTEX_CMP:

		case CELL_GCM_VERTEX_UB256:
			for (u32 j = 0; j < vertex_arrays[index].size(); ++j)
			{
				dump.write(fmt::format("%d\n", vertex_arrays[index][j]));
				if (!((j + 1) % vertex_info.size)) dump.write("\n");
			}
			break;
		}

		dump.write("\n");
#endif
	}

	m_vbo.data(vertex_arrays_data.size(), vertex_arrays_data.data());
	m_vao.bind();

	for (int index = 0; index < rsx::limits::vertex_count; ++index)
	{
		auto &vertex_info = vertex_arrays_info[index];

		if (!vertex_info.size)
		{
			//disabled
			continue;
		}

		if (vertex_info.type < 1 || vertex_info.type > 7)
		{
			LOG_ERROR(RSX, "GLGSRender::EnableVertexData: Bad vertex data type (%d)!", vertex_info.type);
			continue;
		}

		static const std::string reg_table[] =
		{
			"in_pos", "in_weight", "in_normal",
			"in_diff_color", "in_spec_color",
			"in_fog",
			"in_point_size", "in_7",
			"in_tc0", "in_tc1", "in_tc2", "in_tc3",
			"in_tc4", "in_tc5", "in_tc6", "in_tc7"
		};

		int location;

		//TODO: use attrib input mask register
		if (!m_program->attribs.has_location(reg_table[index], &location))
			continue;

		if (vertex_info.array)
		{
			__glcheck m_program->attribs[location] =
				(m_vao + vertex_arrays_offsets[index])
				.config(gl_types[vertex_info.type], vertex_info.size, gl_normalized[vertex_info.type]);
		}
		else
		{
			auto &vertex_data = vertex_arrays[index];

			switch (vertex_info.type)
			{
			case CELL_GCM_VERTEX_F:
				switch (vertex_info.size)
				{
				case 1: apply_attrib_array<f32, 1>(*m_program, location, vertex_data); break;
				case 2: apply_attrib_array<f32, 2>(*m_program, location, vertex_data); break;
				case 3: apply_attrib_array<f32, 3>(*m_program, location, vertex_data); break;
				case 4: apply_attrib_array<f32, 4>(*m_program, location, vertex_data); break;
				}
				break;

			default:
				LOG_ERROR(RSX, "bad non array vertex data format (type = %d, size = %d)", vertex_info.type, vertex_info.size);
				break;
			}
		}
	}

	if (vertex_index_array.empty())
	{
		draw_fbo.draw_arrays(gl::draw_mode(draw_mode - 1), vertex_draw_count);
	}
	else
	{
		m_ebo.data(vertex_index_array.size(), vertex_index_array.data());

		u32 indexed_type = rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4;

		__glcheck glDrawElements(draw_mode - 1, vertex_draw_count,
			(indexed_type == CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT), nullptr);
	}

	write_buffers();

	rsx::thread::end();
}

void GLGSRender::oninit_thread()
{
	GSRender::oninit_thread();

	gl::init();
	LOG_NOTICE(Log::RSX, (const char*)glGetString(GL_VERSION));
	LOG_NOTICE(Log::RSX, (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
	LOG_NOTICE(Log::RSX, (const char*)glGetString(GL_VENDOR));

	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	m_vao.create();
	m_vbo.create();
	m_ebo.create();
	m_scale_offset_buffer.create(16 * sizeof(float));
	m_vertex_constants_buffer.create(512 * 4 * sizeof(float));

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_scale_offset_buffer.id());
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_vertex_constants_buffer.id());

	m_vao.array_buffer = m_vbo;
	m_vao.element_array_buffer = m_ebo;
}

void GLGSRender::onexit_thread()
{
	glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);

	//if (m_program)
	//	m_program.remove();

	if (draw_fbo)
		draw_fbo.remove();

	for (auto &tex : m_draw_tex_color)
		if (tex) tex.remove();

	if (m_draw_tex_depth_stencil)
		m_draw_tex_depth_stencil.remove();

	if (m_flip_fbo)
		m_flip_fbo.remove();

	if (m_flip_tex_color)
		m_flip_tex_color.remove();

	if (m_vbo)
		m_vbo.remove();

	if (m_ebo)
		m_ebo.remove();

	if (m_vao)
		m_vao.remove();

	m_prog_buffer.clear();
}

void nv4097_clear_surface(u32 arg, GLGSRender* renderer)
{
	//LOG_NOTICE(Log::RSX, "nv4097_clear_surface(0x%x)", arg);

	if ((arg & 0xf3) == 0)
	{
		//do nothing
		return;
	}

	renderer->draw_fbo.bind();
	glEnable(GL_SCISSOR_TEST);

	/*
	u16 clear_x = rsx::method_registers[NV4097_SET_CLEAR_RECT_HORIZONTAL];
	u16 clear_y = rsx::method_registers[NV4097_SET_CLEAR_RECT_VERTICAL];
	u16 clear_w = rsx::method_registers[NV4097_SET_CLEAR_RECT_HORIZONTAL] >> 16;
	u16 clear_h = rsx::method_registers[NV4097_SET_CLEAR_RECT_VERTICAL] >> 16;
	glScissor(clear_x, clear_y, clear_w, clear_h);
	*/

	u32 scissor_horizontal = rsx::method_registers[NV4097_SET_SCISSOR_HORIZONTAL];
	u32 scissor_vertical = rsx::method_registers[NV4097_SET_SCISSOR_VERTICAL];
	u16 scissor_x = scissor_horizontal;
	u16 scissor_w = scissor_horizontal >> 16;
	u16 scissor_y = scissor_vertical;
	u16 scissor_h = scissor_vertical >> 16;

	glScissor(scissor_x, scissor_y, scissor_w, scissor_h);

	GLbitfield mask = 0;

	if (arg & 0x1)
	{
		u32 surface_depth_format = (rsx::method_registers[NV4097_SET_SURFACE_FORMAT] >> 5) & 0x7;
		u32 max_depth_value = surface_depth_format == CELL_GCM_SURFACE_Z16 ? 0x0000ffff : 0x00ffffff;

		u32 clear_depth = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] >> 8;

		glDepthMask(GL_TRUE);
		glClearDepth(double(clear_depth) / max_depth_value);
		mask |= GLenum(gl::buffers::depth);
	}

	if (arg & 0x2)
	{
		u8 clear_stencil = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] & 0xff;

		__glcheck glStencilMask(rsx::method_registers[NV4097_SET_STENCIL_MASK]);
		glClearStencil(clear_stencil);

		mask |= GLenum(gl::buffers::stencil);
	}

	if (arg & 0xf0)
	{
		u32 clear_color = rsx::method_registers[NV4097_SET_COLOR_CLEAR_VALUE];
		u8 clear_a = clear_color >> 24;
		u8 clear_r = clear_color >> 16;
		u8 clear_g = clear_color >> 8;
		u8 clear_b = clear_color;

		glColorMask(((arg & 0x20) ? 1 : 0), ((arg & 0x40) ? 1 : 0), ((arg & 0x80) ? 1 : 0), ((arg & 0x10) ? 1 : 0));
		glClearColor(clear_r / 255.f, clear_g / 255.f, clear_b / 255.f, clear_a / 255.f);

		mask |= GLenum(gl::buffers::color);
	}

	renderer->clear_surface_buffers = (gl::buffers)mask;
	renderer->draw_fbo.clear((gl::buffers)mask);
}

using rsx_method_impl_t = void(*)(u32, GLGSRender*);

static const std::unordered_map<u32, rsx_method_impl_t> g_gl_method_tbl =
{
	{ NV4097_CLEAR_SURFACE, nv4097_clear_surface }
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

bool GLGSRender::load_program()
{
#if 1
	RSXVertexProgram vertex_program;
	u32 transform_program_start = rsx::method_registers[NV4097_SET_TRANSFORM_PROGRAM_START];
	vertex_program.data.reserve((512 - transform_program_start) * 4);

	for (int i = transform_program_start; i < 512; ++i)
	{
		vertex_program.data.resize((i - transform_program_start) * 4 + 4);
		memcpy(vertex_program.data.data() + (i - transform_program_start) * 4, transform_program + i * 4, 4 * sizeof(u32));

		D3 d3;
		d3.HEX = transform_program[i * 4 + 3];

		if (d3.end)
			break;
	}

	RSXFragmentProgram fragment_program;
	u32 shader_program = rsx::method_registers[NV4097_SET_SHADER_PROGRAM];
	fragment_program.offset = shader_program & ~0x3;
	fragment_program.addr = rsx::get_address(fragment_program.offset, (shader_program & 0x3) - 1);
	fragment_program.ctrl = rsx::method_registers[NV4097_SET_SHADER_CONTROL];

	__glcheck m_program = m_prog_buffer.getGraphicPipelineState(&vertex_program, &fragment_program, nullptr, nullptr);
	__glcheck m_program->use();

#else
	std::vector<u32> vertex_program;
	u32 transform_program_start = rsx::method_registers[NV4097_SET_TRANSFORM_PROGRAM_START];
	vertex_program.reserve((512 - transform_program_start) * 4);

	for (int i = transform_program_start; i < 512; ++i)
	{
		vertex_program.resize((i - transform_program_start) * 4 + 4);
		memcpy(vertex_program.data() + (i - transform_program_start) * 4, transform_program + i * 4, 4 * sizeof(u32));

		D3 d3;
		d3.HEX = transform_program[i * 4 + 3];

		if (d3.end)
			break;
	}

	u32 shader_program = rsx::method_registers[NV4097_SET_SHADER_PROGRAM];

	std::string fp_shader; ParamArray fp_parr; u32 fp_size;
	GLFragmentDecompilerThread decompile_fp(fp_shader, fp_parr,
		rsx::get_address(shader_program & ~0x3, (shader_program & 0x3) - 1), fp_size, rsx::method_registers[NV4097_SET_SHADER_CONTROL]);

	std::string vp_shader; ParamArray vp_parr;
	GLVertexDecompilerThread decompile_vp(vertex_program, vp_shader, vp_parr);
	decompile_fp.Task();
	decompile_vp.Task();

	LOG_NOTICE(RSX, "fp: %s", fp_shader.c_str());
	LOG_NOTICE(RSX, "vp: %s", vp_shader.c_str());

	static bool first = true;
	gl::glsl::shader fp(gl::glsl::shader::type::fragment, fp_shader);
	gl::glsl::shader vp(gl::glsl::shader::type::vertex, vp_shader);

	(m_program.recreate() += { fp.compile(), vp.compile() }).make();
#endif
	glBindBuffer(GL_UNIFORM_BUFFER, m_scale_offset_buffer.id());

	void *buffer = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	fill_scale_offset_data(buffer, false);
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	glBindBuffer(GL_UNIFORM_BUFFER, m_vertex_constants_buffer.id());
	buffer = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	fill_vertex_program_constants_data(buffer);
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	for (u32 constant_offset : m_prog_buffer.getFragmentConstantOffsetsCache(&fragment_program))
	{
		auto data = vm::ps3::_ptr<u32>(fragment_program.addr + constant_offset);

		u32 c0 = (data[0] >> 16 | data[0] << 16);
		u32 c1 = (data[1] >> 16 | data[1] << 16);
		u32 c2 = (data[2] >> 16 | data[2] << 16);
		u32 c3 = (data[3] >> 16 | data[3] << 16);

		m_program->uniforms["fc" + std::to_string(constant_offset)] = color4f{ (f32&)c0, (f32&)c1, (f32&)c2, (f32&)c3 };
	}

	return true;
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

	u32 clip_horizontal = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL];
	u32 clip_vertical = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL];

	u32 clip_width = clip_horizontal >> 16;
	u32 clip_height = clip_vertical >> 16;
	u32 clip_x = clip_horizontal;
	u32 clip_y = clip_vertical;

	if (!draw_fbo || m_surface.format != surface_format)
	{
		m_surface.unpack(surface_format);
		m_surface.width = clip_width;
		m_surface.height = clip_height;

		LOG_WARNING(RSX, "surface: %dx%d", clip_width, clip_height);

		draw_fbo.recreate();
		m_draw_tex_depth_stencil.recreate(gl::texture::target::texture2D);

		auto format = surface_color_format_to_gl(m_surface.color_format);

		for (int i = 0; i < rsx::limits::color_buffers_count; ++i)
		{
			m_draw_tex_color[i].recreate(gl::texture::target::texture2D);
			__glcheck m_draw_tex_color[i].config()
				.size({ (int)m_surface.width, (int)m_surface.height })
				.type(format.type)
				.format(format.format)
				.swizzle(format.swizzle.r, format.swizzle.g, format.swizzle.b, format.swizzle.a);

			__glcheck m_draw_tex_color[i].pixel_pack_settings().swap_bytes(format.swap_bytes).aligment(1);
			__glcheck m_draw_tex_color[i].pixel_unpack_settings().swap_bytes(format.swap_bytes).aligment(1);

			__glcheck draw_fbo.color[i] = m_draw_tex_color[i];
			__glcheck draw_fbo.check();
		}

		switch (m_surface.depth_format)
		{
		case CELL_GCM_SURFACE_Z16:
		{
			__glcheck m_draw_tex_depth_stencil.config()
				.size({ (int)m_surface.width, (int)m_surface.height })
				.type(gl::texture::type::ushort)
				.format(gl::texture::format::depth)
				.internal_format(gl::texture::internal_format::depth16);

			__glcheck draw_fbo.depth = m_draw_tex_depth_stencil;
			break;
		}

		case CELL_GCM_SURFACE_Z24S8:
		{
			__glcheck m_draw_tex_depth_stencil.config()
				.size({ (int)m_surface.width, (int)m_surface.height })
				.type(gl::texture::type::uint_24_8)
				.format(gl::texture::format::depth_stencil)
				.internal_format(gl::texture::internal_format::depth24_stencil8);

			__glcheck draw_fbo.depth_stencil = m_draw_tex_depth_stencil;
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

		__glcheck m_draw_tex_depth_stencil.pixel_pack_settings().aligment(1);
		__glcheck m_draw_tex_depth_stencil.pixel_unpack_settings().aligment(1);
	}

	if (clear_surface_buffers == gl::buffers::none)
	{
		read_buffers();
	}

	switch (rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET])
	{
	case CELL_GCM_SURFACE_TARGET_NONE: break;

	case CELL_GCM_SURFACE_TARGET_0:
		__glcheck draw_fbo.draw_buffer(draw_fbo.color[0]);
		break;

	case CELL_GCM_SURFACE_TARGET_1:
		__glcheck draw_fbo.draw_buffer(draw_fbo.color[1] );
		break;

	case CELL_GCM_SURFACE_TARGET_MRT1:
		__glcheck draw_fbo.draw_buffers({ draw_fbo.color[0], draw_fbo.color[1] });
		break;

	case CELL_GCM_SURFACE_TARGET_MRT2:
		__glcheck draw_fbo.draw_buffers({ draw_fbo.color[0], draw_fbo.color[1], draw_fbo.color[2] });
		break;

	case CELL_GCM_SURFACE_TARGET_MRT3:
		__glcheck draw_fbo.draw_buffers({ draw_fbo.color[0], draw_fbo.color[1], draw_fbo.color[2], draw_fbo.color[3] });
		break;

	default:
		LOG_ERROR(RSX, "Bad surface color target: %d", rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]);
		break;
	}

	if (clear_surface_buffers != gl::buffers::none)
	{
		//draw_fbo.clear(clear_surface_buffers);

		clear_surface_buffers = gl::buffers::none;
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
	if (!draw_fbo)
		return;

	glDisable(GL_STENCIL_TEST);

	if (rpcs3::state.config.rsx.opengl.read_color_buffers)
	{
		auto color_format = surface_color_format_to_gl(m_surface.color_format);

		auto read_color_buffers = [&](int index, int count)
		{
			for (int i = index; i < index + count; ++i)
			{
				u32 color_address = rsx::get_address(rsx::method_registers[mr_color_offset[i]], rsx::method_registers[mr_color_dma[i]]);
				__glcheck m_draw_tex_color[i].copy_from(vm::base(color_address), color_format.format, color_format.type);
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
	}

	if (rpcs3::state.config.rsx.opengl.read_depth_buffer)
	{
		auto depth_format = surface_depth_format_to_gl(m_surface.depth_format);

		int pixel_size = m_surface.depth_format == CELL_GCM_SURFACE_Z16 ? 2 : 4;

		gl::buffer pbo_depth;

		__glcheck pbo_depth.create(m_surface.width * m_surface.height * pixel_size);
		__glcheck pbo_depth.map([&](GLubyte* pixels)
		{
			u32 depth_address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET], rsx::method_registers[NV4097_SET_CONTEXT_DMA_ZETA]);

			if (m_surface.depth_format == CELL_GCM_SURFACE_Z16)
			{
				u16 *dst = (u16*)pixels;
				const be_t<u16>* src = vm::ps3::_ptr<u16>(depth_address);
				for (int i = 0, end = m_draw_tex_depth_stencil.width() * m_draw_tex_depth_stencil.height(); i < end; ++i)
				{
					dst[i] = src[i];
				}
			}
			else
			{
				u32 *dst = (u32*)pixels;
				const be_t<u32>* src = vm::ps3::_ptr<u32>(depth_address);
				for (int i = 0, end = m_draw_tex_depth_stencil.width() * m_draw_tex_depth_stencil.height(); i < end; ++i)
				{
					dst[i] = src[i];
				}
			}
		}, gl::buffer::access::write);

		__glcheck m_draw_tex_depth_stencil.copy_from(pbo_depth, depth_format.second, depth_format.first);
	}
}

void GLGSRender::write_buffers()
{
	if (!draw_fbo)
		return;

	if (rpcs3::state.config.rsx.opengl.write_color_buffers)
	{
		//gl::buffer pbo_color;
		//__glcheck pbo_color.create(m_draw_tex_color[0].width() * m_draw_tex_color[0].height() * 4);

		auto color_format = surface_color_format_to_gl(m_surface.color_format);

		auto write_color_buffers = [&](int index, int count)
		{
			for (int i = index; i < index + count; ++i)
			{
				//TODO: swizzle
				//__glcheck m_draw_tex_color[i].copy_to(pbo_color, color_format.format, color_format.type);

				//pbo_color.map([&](GLubyte* pixels)
				//{
				//	u32 color_address = rsx::get_address(rsx::method_registers[mr_color_offset[i]], rsx::method_registers[mr_color_dma[i]]);
				//	//u32 depth_address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET], rsx::method_registers[NV4097_SET_CONTEXT_DMA_ZETA]);

				//	const u32 *src = (const u32*)pixels;
				//	be_t<u32>* dst = vm::ps3::_ptr<u32>(color_address);
				//	for (int i = 0, end = m_draw_tex_color[i].width() * m_draw_tex_color[i].height(); i < end; ++i)
				//	{
				//		dst[i] = src[i];
				//	}

				//}, gl::buffer::access::read);

				u32 color_address = rsx::get_address(rsx::method_registers[mr_color_offset[i]], rsx::method_registers[mr_color_dma[i]]);
				__glcheck m_draw_tex_color[i].copy_to(vm::base(color_address), color_format.format, color_format.type);
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
	}

	if (rpcs3::state.config.rsx.opengl.write_depth_buffer)
	{
		auto depth_format = surface_depth_format_to_gl(m_surface.depth_format);

		gl::buffer pbo_depth;

		int pixel_size = m_surface.depth_format == CELL_GCM_SURFACE_Z16 ? 2 : 4;

		__glcheck pbo_depth.create(m_surface.width * m_surface.height * pixel_size);
		__glcheck m_draw_tex_depth_stencil.copy_to(pbo_depth, depth_format.second, depth_format.first);

		__glcheck pbo_depth.map([&](GLubyte* pixels)
		{
			u32 depth_address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET], rsx::method_registers[NV4097_SET_CONTEXT_DMA_ZETA]);

			if (m_surface.depth_format == CELL_GCM_SURFACE_Z16)
			{
				const u16 *src = (const u16*)pixels;
				be_t<u16>* dst = vm::ps3::_ptr<u16>(depth_address);
				for (int i = 0, end = m_draw_tex_depth_stencil.width() * m_draw_tex_depth_stencil.height(); i < end; ++i)
				{
					dst[i] = src[i];
				}
			}
			else
			{
				const u32 *src = (const u32*)pixels;
				be_t<u32>* dst = vm::ps3::_ptr<u32>(depth_address);
				for (int i = 0, end = m_draw_tex_depth_stencil.width() * m_draw_tex_depth_stencil.height(); i < end; ++i)
				{
					dst[i] = src[i];
				}
			}

		}, gl::buffer::access::read);
	}
}

void GLGSRender::flip(int buffer)
{
	//LOG_NOTICE(Log::RSX, "flip(%d)", buffer);
	u32 buffer_width = gcm_buffers[buffer].width;
	u32 buffer_height = gcm_buffers[buffer].height;
	u32 buffer_pitch = gcm_buffers[buffer].pitch;
	u32 buffer_address = rsx::get_address(gcm_buffers[buffer].offset, CELL_GCM_LOCATION_LOCAL);
	bool skip_read = false;

	if (draw_fbo && !rpcs3::state.config.rsx.opengl.write_color_buffers)
	{
		skip_read = true;
		/*
		for (uint i = 0; i < rsx::limits::color_buffers_count; ++i)
		{
			u32 color_address = rsx::get_address(rsx::method_registers[mr_color_offset[i]], rsx::method_registers[mr_color_dma[i]]);

			if (color_address == buffer_address)
			{
				skip_read = true;
				__glcheck draw_fbo.draw_buffer(draw_fbo.color[i]);
				break;
			}
		}
		*/
	}

	if (!skip_read)
	{
		if (!m_flip_tex_color || m_flip_tex_color.size() != sizei{ (int)buffer_width, (int)buffer_height })
		{
			m_flip_tex_color.recreate(gl::texture::target::texture2D);

			__glcheck m_flip_tex_color.config()
				.size({ (int)buffer_width, (int)buffer_height })
				.type(gl::texture::type::uint_8_8_8_8)
				.format(gl::texture::format::bgra);

			m_flip_tex_color.pixel_unpack_settings().aligment(1);
			m_flip_tex_color.pixel_pack_settings().aligment(1);

			__glcheck m_flip_fbo.recreate();
			__glcheck m_flip_fbo.color = m_flip_tex_color;
		}

		__glcheck m_flip_fbo.draw_buffer(m_flip_fbo.color);

		m_flip_fbo.bind();

		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_LOGIC_OP);
		glDisable(GL_CULL_FACE);

		__glcheck m_flip_tex_color.copy_from(vm::base(buffer_address), gl::texture::format::bgra, gl::texture::type::uint_8_8_8_8);
	}

	areai screen_area = coordi({}, { (int)buffer_width, (int)buffer_height });

	coordi aspect_ratio;
	if (1) //enable aspect ratio
	{
		sizei csize = m_frame->client_size();
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
		aspect_ratio.size = m_frame->client_size();
	}

	gl::screen.clear(gl::buffers::color_depth_stencil);

	if (!skip_read)
	{
		__glcheck m_flip_fbo.blit(gl::screen, screen_area, areai(aspect_ratio).flipped_vertical());
	}
	else
	{
		__glcheck draw_fbo.blit(gl::screen, screen_area, areai(aspect_ratio).flipped_vertical());
	}

	m_frame->flip(m_context);
}


u64 GLGSRender::timestamp() const
{
	GLint64 result;
	glGetInteger64v(GL_TIMESTAMP, &result);
	return result;
}
