#include "extension_types.h"

namespace rpcs3
{
	inline namespace core
	{
		f16::f16(f32 value) : _u16(f32_to_f16(value)._u16)
		{
		}

		f16::operator f32() const
		{
			return f16_to_f32(*this);
		}

		f16 f16::make(u16 raw)
		{
			f16 result;
			result._u16 = raw;
			return result;
		}

		f16 f16::f32_to_f16(f32 value)
		{
			u32 bits = (u32&)value;

			if (bits == 0)
				return make(0);

			s32 e = ((bits & 0x7f800000) >> 23) - 127 + 15;

			if (e < 0)
				return make(0);

			if (e > 31)
				e = 31;

			u32 s = bits & 0x80000000;
			u32 m = bits & 0x007fffff;

			return make(((s >> 16) & 0x8000) | ((e << 10) & 0x7c00) | ((m >> 13) & 0x03ff));
		}

		f32 f16::f16_to_f32(f16 value)
		{
			if (value._u16 == 0)
				return 0.0f;

			u32 s = value._u16 & 0x8000;
			s32 e = ((value._u16 & 0x7c00) >> 10) - 15 + 127;
			u32 m = value._u16 & 0x03ff;
			u32 result = (s << 16) | ((e << 23) & 0x7f800000) | (m << 13);

			return (f32&)result;
		}
	}
}