#include "stdafx.h"
#include "rsx_gl_texture.h"
#include "gl_helpers.h"
#include "../GCM.h"
#include "../RSXThread.h"
#include "../RSXTexture.h"
#include "../rsx_utils.h"
#include "../Common/TextureUtils.h"
#include "gl_texture_cache.h"

const std::array<GLint, 4> default_remap{ GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE };
const std::array<GLint, 4> remap_B8{ GL_RED, GL_RED, GL_RED, GL_RED };
const std::array<GLint, 4> remap_G8B8{ GL_RED, GL_GREEN, GL_RED, GL_GREEN };
const std::array<GLint, 4> remap_R6G5B5{ GL_ALPHA, GL_GREEN, GL_RED, GL_BLUE };
const std::array<GLint, 4> remap_X16{ GL_RED, GL_ONE, GL_RED, GL_ONE };
const std::array<GLint, 4> remap_Y16_X16{ GL_GREEN, GL_RED, GL_GREEN, GL_RED };
const std::array<GLint, 4> remap_FLOAT{ GL_RED, GL_ONE, GL_ONE, GL_ONE };
const std::array<GLint, 4> remap_X32_FLOAT{ GL_RED, GL_ONE, GL_ONE, GL_ONE };
const std::array<GLint, 4> remap_D1R5G5B5{ GL_ONE, GL_RED, GL_GREEN, GL_BLUE };
const std::array<GLint, 4> remap_D8R8G8B8{ GL_ONE, GL_RED, GL_GREEN, GL_BLUE };
const std::array<GLint, 4> remap_Y16_X16_FLOAT{ GL_RED, GL_GREEN, GL_RED, GL_GREEN };

const std::unordered_map<u32, gl::texture_format> textures_fromats
{
	{ CELL_GCM_TEXTURE_B8,{ 1, remap_B8, gl::texture::sized_internal_format::r8, gl::texture::format::red, gl::texture::type::ubyte, gl::texture_flags::none } },
	{ CELL_GCM_TEXTURE_A1R5G5B5,{ 2, default_remap, gl::texture::sized_internal_format::rgb5_a1, gl::texture::format::bgra, gl::texture::type::ushort_5_5_5_1, gl::texture_flags::allow_remap | gl::texture_flags::allow_swizzle } },
	{ CELL_GCM_TEXTURE_A4R4G4B4,{ 2, default_remap, gl::texture::sized_internal_format::rgba4, gl::texture::format::bgra, gl::texture::type::ushort_4_4_4_4, gl::texture_flags::allow_remap | gl::texture_flags::allow_swizzle } },
	{ CELL_GCM_TEXTURE_R5G6B5,{ 2, default_remap, gl::texture::sized_internal_format::rgb565, gl::texture::format::rgb, gl::texture::type::ushort_5_6_5, gl::texture_flags::allow_remap | gl::texture_flags::allow_swizzle | gl::texture_flags::swap_bytes} },
	{ CELL_GCM_TEXTURE_A8R8G8B8,{ 4, default_remap, gl::texture::sized_internal_format::rgba8, gl::texture::format::bgra, gl::texture::type::uint_8_8_8_8, gl::texture_flags::allow_remap | gl::texture_flags::allow_swizzle } },
	{ CELL_GCM_TEXTURE_G8B8,{ 2, remap_G8B8, gl::texture::sized_internal_format::rg8, gl::texture::format::rg, gl::texture::type::ubyte, gl::texture_flags::allow_remap } },
	{ CELL_GCM_TEXTURE_R6G5B5,{ 2, remap_R6G5B5, gl::texture::sized_internal_format::rgb565, gl::texture::format::rgb, gl::texture::type::ushort_5_6_5, gl::texture_flags::allow_remap | gl::texture_flags::allow_swizzle | gl::texture_flags::swap_bytes } },
	{ CELL_GCM_TEXTURE_DEPTH24_D8,{ 4, default_remap, gl::texture::sized_internal_format::depth24, gl::texture::format::depth, gl::texture::type::uint_8_8_8_8_rev, gl::texture_flags::allow_remap } },
	{ CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT,{ 4, default_remap, gl::texture::sized_internal_format::depth24, gl::texture::format::depth, gl::texture::type::f32, gl::texture_flags::allow_remap } },
	{ CELL_GCM_TEXTURE_DEPTH16,{ 2, default_remap, gl::texture::sized_internal_format::depth16, gl::texture::format::depth, gl::texture::type::ushort, gl::texture_flags::allow_remap } },
	{ CELL_GCM_TEXTURE_DEPTH16_FLOAT,{ 2, default_remap, gl::texture::sized_internal_format::depth16, gl::texture::format::depth, gl::texture::type::f32, gl::texture_flags::allow_remap } },
	{ CELL_GCM_TEXTURE_X16,{ 2, remap_X16, gl::texture::sized_internal_format::r16ui, gl::texture::format::red, gl::texture::type::ushort, gl::texture_flags::swap_bytes } },
	{ CELL_GCM_TEXTURE_Y16_X16,{ 4, remap_Y16_X16, gl::texture::sized_internal_format::rg16ui, gl::texture::format::rg, gl::texture::type::ushort, gl::texture_flags::allow_remap | gl::texture_flags::swap_bytes } },
	{ CELL_GCM_TEXTURE_R5G5B5A1,{ 2, default_remap, gl::texture::sized_internal_format::rgb5_a1, gl::texture::format::rgba, gl::texture::type::ushort_5_5_5_1, gl::texture_flags::allow_remap | gl::texture_flags::swap_bytes | gl::texture_flags::allow_swizzle } },
	{ CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT,{ 8, default_remap, gl::texture::sized_internal_format::rgba16f, gl::texture::format::rgba, gl::texture::type::f16, gl::texture_flags::allow_remap | gl::texture_flags::swap_bytes } },
	{ CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT,{ 16, default_remap, gl::texture::sized_internal_format::rgba32f, gl::texture::format::rgba, gl::texture::type::f32, gl::texture_flags::allow_remap | gl::texture_flags::swap_bytes } },
	{ CELL_GCM_TEXTURE_X32_FLOAT,{ 4, remap_X32_FLOAT, gl::texture::sized_internal_format::r32f, gl::texture::format::red, gl::texture::type::f32, gl::texture_flags::swap_bytes } },
	{ CELL_GCM_TEXTURE_D1R5G5B5,{ 2, remap_D1R5G5B5, gl::texture::sized_internal_format::rgb5_a1, gl::texture::format::bgra, gl::texture::type::ushort_5_5_5_1, gl::texture_flags::allow_remap | gl::texture_flags::allow_swizzle } },
	{ CELL_GCM_TEXTURE_D8R8G8B8,{ 4, remap_D8R8G8B8, gl::texture::sized_internal_format::rgba8, gl::texture::format::bgra, gl::texture::type::uint_8_8_8_8, gl::texture_flags::allow_remap | gl::texture_flags::allow_swizzle } },
	{ CELL_GCM_TEXTURE_Y16_X16_FLOAT,{ 4, remap_Y16_X16_FLOAT, gl::texture::sized_internal_format::rg16f, gl::texture::format::rg, gl::texture::type::f16, gl::texture_flags::allow_remap | gl::texture_flags::swap_bytes } },
};

namespace gl
{
	const texture_format& get_texture_format(u32 texture_id)
	{
		return textures_fromats.at(texture_id);
	}
}

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

float max_aniso(int aniso)
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


bool compressed_format(u32 format)
{
	switch (format)
	{
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		return true;
	}

	return false;
}

int wrap(int wrap)
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


void rsx::gl_texture::bind(gl::texture_cache& cache, rsx::texture& tex)
{
	u32 full_format = tex.format();
	u32 format = full_format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
	bool is_compressed = compressed_format(format);
	bool is_swizzled;
	bool is_normalized;

	if (is_compressed)
	{
		is_swizzled = false;
		is_normalized = true;
	}
	else
	{
		is_swizzled = (~full_format & CELL_GCM_TEXTURE_LN) != 0;
		is_normalized = (~full_format & CELL_GCM_TEXTURE_UN) != 0;
	}

	gl::texture::target target = is_normalized ? gl::texture::target::texture2D : gl::texture::target::texture_rectangle;

	glActiveTexture(GL_TEXTURE0 + tex.index());
	gl::texture_view(target, 0).bind();

	if (!tex.enabled())
	{
		return;
	}

	const GLint *remap = default_remap.data();

	//TODO
	gl::texture_info info{};

	info.width = std::max<u16>(tex.width(), 1);
	info.height = std::max<u16>(tex.height(), 1);
	info.depth = std::max<u16>(tex.depth(), 1);
	info.dimension = tex.dimension();
	info.start_address = rsx::get_address(tex.offset(), tex.location());
	info.swizzled = is_swizzled;
	info.target = target;
	info.mipmap = target != gl::texture::target::texture_rectangle ? tex.mipmap() : 1;

	if (info.mipmap > 1)
	{
		info.min_lod = tex.min_lod() >> 8;
		info.max_lod = tex.max_lod() >> 8;
		info.lod_bias = tex.bias();
	}

	gl::texture_flags flags = gl::texture_flags::none;

	if (is_compressed)
	{
		info.format.type = gl::texture::type::ubyte;
		info.format.format = gl::texture::format::rgba;
		info.pitch = 0;

		switch (format)
		{
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1: // Compressed 4x4 pixels into 8 bytes
			info.compressed_size = ((info.width + 3) / 4) * ((info.height + 3) / 4) * 8;
			info.format.internal_format = gl::texture::sized_internal_format::compressed_rgba_s3tc_dxt1;
			break;

		case CELL_GCM_TEXTURE_COMPRESSED_DXT23: // Compressed 4x4 pixels into 16 bytes
			info.compressed_size = ((info.width + 3) / 4) * ((info.height + 3) / 4) * 16;
			info.format.internal_format = gl::texture::sized_internal_format::compressed_rgba_s3tc_dxt3;
			break;

		case CELL_GCM_TEXTURE_COMPRESSED_DXT45: // Compressed 4x4 pixels into 16 bytes
			info.compressed_size = ((info.width + 3) / 4) * ((info.height + 3) / 4) * 16;
			info.format.internal_format = gl::texture::sized_internal_format::compressed_rgba_s3tc_dxt5;
			break;

		default:
			throw EXCEPTION("bad compressed format");
		}

		info.format.bpp = 1;
		info.format.format = gl::texture::format::rgba;
		info.format.flags = gl::texture_flags::none;
	}
	else
	{
		auto found = textures_fromats.find(format);

		if (found == textures_fromats.end())
		{
			throw EXCEPTION("unimplemented texture format 0x%x", format);
		}

		info.swizzled = info.swizzled && (found->second.flags & gl::texture_flags::allow_swizzle) != gl::texture_flags::none;
		info.format = found->second;
		info.pitch = std::max(info.width * info.format.bpp, tex.pitch());
		flags = found->second.flags;
		info.format.flags &= gl::texture_flags::allow_swizzle;

		remap = info.format.remap.data();
	}

	__glcheck cache.entry(info, gl::cache_buffers::local).bind(tex.index());

	if ((flags & gl::texture_flags::allow_remap) != gl::texture_flags::none)
	{
		u8 remap_a = tex.remap() & 0x3;
		u8 remap_r = (tex.remap() >> 2) & 0x3;
		u8 remap_g = (tex.remap() >> 4) & 0x3;
		u8 remap_b = (tex.remap() >> 6) & 0x3;

		__glcheck glTexParameteri((GLenum)target, GL_TEXTURE_SWIZZLE_A, remap[remap_a]);
		__glcheck glTexParameteri((GLenum)target, GL_TEXTURE_SWIZZLE_R, remap[remap_r]);
		__glcheck glTexParameteri((GLenum)target, GL_TEXTURE_SWIZZLE_G, remap[remap_g]);
		__glcheck glTexParameteri((GLenum)target, GL_TEXTURE_SWIZZLE_B, remap[remap_b]);
	}
	else
	{
		__glcheck glTexParameteri((GLenum)target, GL_TEXTURE_SWIZZLE_A, remap[0]);
		__glcheck glTexParameteri((GLenum)target, GL_TEXTURE_SWIZZLE_R, remap[1]);
		__glcheck glTexParameteri((GLenum)target, GL_TEXTURE_SWIZZLE_G, remap[2]);
		__glcheck glTexParameteri((GLenum)target, GL_TEXTURE_SWIZZLE_B, remap[3]);
	}

	__glcheck glTexParameteri((GLenum)target, GL_TEXTURE_WRAP_S, wrap(tex.wrap_s()));
	__glcheck glTexParameteri((GLenum)target, GL_TEXTURE_WRAP_T, wrap(tex.wrap_t()));
	__glcheck glTexParameteri((GLenum)target, GL_TEXTURE_WRAP_R, wrap(tex.wrap_r()));

	__glcheck glTexParameteri((GLenum)target, GL_TEXTURE_COMPARE_FUNC, gl_tex_zfunc[tex.zfunc()]);

	__glcheck glTexParameteri((GLenum)target, GL_TEXTURE_MIN_FILTER, gl_tex_min_filter[tex.min_filter()]);
	__glcheck glTexParameteri((GLenum)target, GL_TEXTURE_MAG_FILTER, gl_tex_mag_filter[tex.mag_filter()]);
	__glcheck glTexParameterf((GLenum)target, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso(tex.max_aniso()));
}

