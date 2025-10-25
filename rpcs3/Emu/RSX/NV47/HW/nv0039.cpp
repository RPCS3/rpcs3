#include "stdafx.h"
#include "nv0039.h"

#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/Core/RSXReservationLock.hpp"
#include "Emu/RSX/Host/MM.h"

#include "Utilities/deferred_op.hpp"

#include "context_accessors.define.h"

namespace rsx
{
	namespace nv0039
	{
		// Transfer with stride
		inline void block2d_copy_with_stride(u8* dst, const u8* src, u32 width, u32 height, u32 src_pitch, u32 dst_pitch, u8 src_stride, u8 dst_stride)
		{
			for (u32 row = 0; row < height; ++row)
			{
				auto dst_ptr = dst;
				auto src_ptr = src;
				while (src_ptr < src + width)
				{
					*dst_ptr = *src_ptr;

					src_ptr += src_stride;
					dst_ptr += dst_stride;
				}

				dst += dst_pitch;
				src += src_pitch;
			}
		}

		inline void block2d_copy(u8* dst, const u8* src, u32 width, u32 height, u32 src_pitch, u32 dst_pitch)
		{
			for (u32 i = 0; i < height; ++i)
			{
				std::memcpy(dst, src, width);
				dst += dst_pitch;
				src += src_pitch;
			}
		}

		void buffer_notify(context* ctx, u32, u32 arg)
		{
			s32 in_pitch = REGS(ctx)->nv0039_input_pitch();
			s32 out_pitch = REGS(ctx)->nv0039_output_pitch();
			const u32 line_length = REGS(ctx)->nv0039_line_length();
			const u32 line_count = REGS(ctx)->nv0039_line_count();
			const u8 out_format = REGS(ctx)->nv0039_output_format();
			const u8 in_format = REGS(ctx)->nv0039_input_format();
			const u32 notify = arg;

			if (!line_count || !line_length)
			{
				rsx_log.warning("NV0039_BUFFER_NOTIFY NOPed out: pitch(in=0x%x, out=0x%x), line(len=0x%x, cnt=0x%x), fmt(in=0x%x, out=0x%x), notify=0x%x",
					in_pitch, out_pitch, line_length, line_count, in_format, out_format, notify);
				return;
			}

			rsx_log.trace("NV0039_BUFFER_NOTIFY: pitch(in=0x%x, out=0x%x), line(len=0x%x, cnt=0x%x), fmt(in=0x%x, out=0x%x), notify=0x%x",
				in_pitch, out_pitch, line_length, line_count, in_format, out_format, notify);

			u32 src_offset = REGS(ctx)->nv0039_input_offset();
			u32 src_dma = REGS(ctx)->nv0039_input_location();

			u32 dst_offset = REGS(ctx)->nv0039_output_offset();
			u32 dst_dma = REGS(ctx)->nv0039_output_location();

			const bool is_block_transfer = (in_pitch == out_pitch && out_pitch + 0u == line_length);
			const auto read_address = get_address(src_offset, src_dma);
			const auto write_address = get_address(dst_offset, dst_dma);
			const auto read_length = in_pitch * (line_count - 1) + line_length;
			const auto write_length = out_pitch * (line_count - 1) + line_length;

			RSX(ctx)->invalidate_fragment_program(dst_dma, dst_offset, write_length);

			if (const auto result = RSX(ctx)->read_barrier(read_address, read_length, !is_block_transfer);
				result == rsx::result_zcull_intr)
			{
				// This transfer overlaps will zcull data pool
				if (RSX(ctx)->copy_zcull_stats(read_address, read_length, write_address) == write_length)
				{
					// All writes deferred
					return;
				}
			}

			// Deferred write_barrier on RSX side
			utils::deferred_op deferred([&]()
			{
				RSX(ctx)->write_barrier(write_address, write_length);
				// res->release(0);
			});

			auto res = ::rsx::reservation_lock<true>(write_address, write_length, read_address, read_length);

			u8* dst = vm::_ptr<u8>(write_address);
			const u8* src = vm::_ptr<u8>(read_address);

			rsx::simple_array<utils::address_range64> flush_mm_ranges =
			{
				utils::address_range64::start_length(reinterpret_cast<u64>(dst), write_length),
				utils::address_range64::start_length(reinterpret_cast<u64>(src), read_length)
			};
			rsx::mm_flush(flush_mm_ranges);

			const bool is_overlapping = dst_dma == src_dma && [&]() -> bool
			{
				const u32 src_max = src_offset + read_length;
				const u32 dst_max = dst_offset + (out_pitch * (line_count - 1) + line_length);
				return (src_offset >= dst_offset && src_offset < dst_max) ||
				 (dst_offset >= src_offset && dst_offset < src_max);
			}();

			if (in_format > 1 || out_format > 1) [[ unlikely ]]
			{
				// The formats are just input channel strides. You can use this to do cool tricks like gathering channels
				// Very rare, only seen in use by Destiny
				// TODO: Hw accel
				block2d_copy_with_stride(dst, src, line_length, line_count, in_pitch, out_pitch, in_format, out_format);
				return;
			}

			if (!is_overlapping)
			{
				if (is_block_transfer)
				{
					std::memcpy(dst, src, read_length);
					return;
				}

				block2d_copy(dst, src, line_length, line_count, in_pitch, out_pitch);
				return;
			}

			if (is_block_transfer)
			{
				std::memmove(dst, src, read_length);
				return;
			}

			// Handle overlapping 2D range using double-copy to temp.
			std::vector<u8> temp(line_length * line_count);
			u8* buf = temp.data();

			block2d_copy(buf, src, line_length, line_count, in_pitch, line_length);
			block2d_copy(dst, buf, line_length, line_count, line_length, out_pitch);
		}
	}
}
