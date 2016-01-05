#include "stdafx.h"
#include "rsx_utils.h"

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
}
