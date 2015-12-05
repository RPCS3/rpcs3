#pragma once
#include "basic_types.h"

namespace common
{
	union alignas(2) f16
	{
		u16 _u16;
		u8 _u8[2];

		f16() = default;
		f16(f32 value);

		operator f32() const;

	private:
		static f16 make(u16 raw);
		static f16 f32_to_f16(f32 value);
		static f32 f16_to_f32(f16 value);
	};
}