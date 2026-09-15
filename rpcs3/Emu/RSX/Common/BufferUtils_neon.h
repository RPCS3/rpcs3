#pragma once

#include <arm_neon.h>

namespace
{
	template <bool Compare>
	auto copy_data_swap_u32_neon(u32* dst, const u32* src, u32 count)
	{
		uint32x4_t changed = vdupq_n_u32(0);
		u32 tail_changed = 0;
		u32 i = 0;

		for (; count - i >= 4; i += 4)
		{
			const auto value = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(reinterpret_cast<const u8*>(src + i))));

			if constexpr (Compare)
			{
				changed = vorrq_u32(changed, veorq_u32(value, vld1q_u32(dst + i)));
			}

			vst1q_u32(dst + i, value);
		}

		for (; i < count; ++i)
		{
			const u32 value = stx::se_storage<u32>::swap(src[i]);

			if constexpr (Compare)
			{
				tail_changed |= value ^ dst[i];
			}

			dst[i] = value;
		}

		if constexpr (Compare)
		{
			return (vmaxvq_u32(changed) | tail_changed) != 0;
		}
	}

	template <typename T, bool Restart>
	u64 upload_untouched_neon(const be_t<T>* src, T* dst, u32 count, T restart_index = 0)
	{
		T min_index = static_cast<T>(-1);
		T max_index = 0;
		u32 i = 0;

		if constexpr (sizeof(T) == 2)
		{
			auto min = vdupq_n_u16(-1);
			auto max = vdupq_n_u16(0);
			const auto restart = vdupq_n_u16(restart_index);

			for (; count - i >= 8; i += 8)
			{
				auto value = vreinterpretq_u16_u8(vrev16q_u8(vld1q_u8(reinterpret_cast<const u8*>(src + i))));

				if constexpr (Restart)
				{
					const auto mask = vceqq_u16(value, restart);
					max = vmaxq_u16(max, vbicq_u16(value, mask));
					value = vorrq_u16(value, mask);
				}
				else
				{
					max = vmaxq_u16(max, value);
				}

				min = vminq_u16(min, value);
				vst1q_u16(dst + i, value);
			}

			min_index = vminvq_u16(min);
			max_index = vmaxvq_u16(max);
		}
		else
		{
			auto min = vdupq_n_u32(-1);
			auto max = vdupq_n_u32(0);
			const auto restart = vdupq_n_u32(restart_index);

			for (; count - i >= 4; i += 4)
			{
				auto value = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(reinterpret_cast<const u8*>(src + i))));

				if constexpr (Restart)
				{
					const auto mask = vceqq_u32(value, restart);
					max = vmaxq_u32(max, vbicq_u32(value, mask));
					value = vorrq_u32(value, mask);
				}
				else
				{
					max = vmaxq_u32(max, value);
				}

				min = vminq_u32(min, value);
				vst1q_u32(dst + i, value);
			}

			min_index = vminvq_u32(min);
			max_index = vmaxvq_u32(max);
		}

		for (; i < count; ++i)
		{
			const T value = src[i];
			if (Restart && value == restart_index)
			{
				dst[i] = static_cast<T>(-1);
			}
			else
			{
				min_index = std::min(min_index, value);
				max_index = std::max(max_index, value);
				dst[i] = value;
			}
		}

		return (u64{max_index} << 32) | min_index;
	}

	void iota16_neon(u16* dst, u32 count)
	{
		const u16 initial[] = {0, 1, 2, 3, 4, 5, 6, 7};
		auto value = vld1q_u16(initial);
		u32 i = 0;

		for (; count - i >= 8; i += 8)
		{
			vst1q_u16(dst + i, value);
			value = vaddq_u16(value, vdupq_n_u16(8));
		}

		for (; i < count; ++i)
		{
			dst[i] = static_cast<u16>(i);
		}
	}
} // namespace
