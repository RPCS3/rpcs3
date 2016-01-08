#include "stdafx.h"
#include "rsx_utils.h"
#include "Common/BufferUtils.h"
#include "rsx_methods.h"

extern "C"
{
#include "libswscale/swscale.h"
}

namespace rsx
{
	void convert_scale_image(u8 *dst, AVPixelFormat dst_format, int dst_width, int dst_height, int dst_pitch,
		const u8 *src, AVPixelFormat src_format, int src_width, int src_height, int src_pitch, int src_slice_h, bool bilinear)
	{
		std::unique_ptr<SwsContext, void(*)(SwsContext*)> sws(sws_getContext(src_width, src_height, src_format,
			dst_width, dst_height, dst_format, bilinear ? SWS_FAST_BILINEAR : SWS_POINT, NULL, NULL, NULL), sws_freeContext);

		sws_scale(sws.get(), &src, &src_pitch, 0, src_slice_h, &dst, &dst_pitch);
	}

	void convert_scale_image(std::unique_ptr<u8[]>& dst, AVPixelFormat dst_format, int dst_width, int dst_height, int dst_pitch,
		const u8 *src, AVPixelFormat src_format, int src_width, int src_height, int src_pitch, int src_slice_h, bool bilinear)
	{
		dst.reset(new u8[dst_pitch * dst_height]);
		convert_scale_image(dst.get(), dst_format, dst_width, dst_height, dst_pitch,
			src, src_format, src_width, src_height, src_pitch, src_slice_h, bilinear);
	}

	void clip_image(u8 *dst, const u8 *src, int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch)
	{
		for (int y = 0; y < clip_h; ++y)
		{
			u8 *dst_row = dst + y * dst_pitch;
			const u8 *src_row = src + (y + clip_y) * src_pitch + clip_x * bpp;

			std::memmove(dst_row, src_row, clip_w * bpp);
		}
	}

	void clip_image(std::unique_ptr<u8[]>& dst, const u8 *src,
		int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch)
	{
		dst.reset(new u8[clip_h * dst_pitch]);
		clip_image(dst.get(), src, clip_x, clip_y, clip_w, clip_h, bpp, src_pitch, dst_pitch);
	}

	void fill_scale_offset_matrix(void *dest_, bool transpose,
		float offset_x, float offset_y, float offset_z,
		float scale_x, float scale_y, float scale_z)
	{
		char *dest = (char*)dest_;

		if (transpose)
		{
			stream_vector(dest + 4 * sizeof(f32) * 0, scale_x, 0, 0, 0);
			stream_vector(dest + 4 * sizeof(f32) * 1, 0, scale_y, 0, 0);
			stream_vector(dest + 4 * sizeof(f32) * 2, 0, 0, scale_z, 0);
			stream_vector(dest + 4 * sizeof(f32) * 3, offset_x, offset_y, offset_z, 1);
		}
		else
		{
			stream_vector(dest + 4 * sizeof(f32) * 0, scale_x, 0, 0, offset_x);
			stream_vector(dest + 4 * sizeof(f32) * 1, 0, scale_y, 0, offset_y);
			stream_vector(dest + 4 * sizeof(f32) * 2, 0, 0, scale_z, offset_z);
			stream_vector(dest + 4 * sizeof(f32) * 3, 0, 0, 0, 1);
		}
	}

	void fill_window_matrix(void *dest, bool transpose)
	{
		u32 shader_window = method_registers[NV4097_SET_SHADER_WINDOW];

		u16 height = shader_window & 0xfff;
		u8 origin = (shader_window >> 12) & 0xf;
		u16 pixelCenter = shader_window >> 16;

		f32 offset_x = method_registers[NV4097_SET_WINDOW_OFFSET] & 0xffff;
		f32 offset_y = method_registers[NV4097_SET_WINDOW_OFFSET] >> 16;
		f32 scale_y = 1.0;

		if (origin == CELL_GCM_WINDOW_ORIGIN_BOTTOM)
		{
			offset_y = height - 1 - offset_y;
			scale_y = -1.0f;
		}

		if (false && pixelCenter == CELL_GCM_WINDOW_PIXEL_CENTER_HALF)
		{
			offset_x += 0.5f;
			offset_y += 0.5f;
		}

		fill_scale_offset_matrix(dest, transpose, offset_x, offset_y, 0.0f, 1.0f, scale_y, 1.0f);
	}

	void fill_viewport_matrix(void *buffer, bool transpose)
	{
		f32 offset_x = (f32&)method_registers[NV4097_SET_VIEWPORT_OFFSET + 0];
		f32 offset_y = (f32&)method_registers[NV4097_SET_VIEWPORT_OFFSET + 1];
		f32 offset_z = (f32&)method_registers[NV4097_SET_VIEWPORT_OFFSET + 2];

		f32 scale_x = (f32&)method_registers[NV4097_SET_VIEWPORT_SCALE + 0];
		f32 scale_y = (f32&)method_registers[NV4097_SET_VIEWPORT_SCALE + 1];
		f32 scale_z = (f32&)method_registers[NV4097_SET_VIEWPORT_SCALE + 2];

		fill_scale_offset_matrix(buffer, transpose, offset_x, offset_y, offset_z, scale_x, scale_y, scale_z);
	}
}
