#pragma once

#include <util/types.hpp>

namespace rpcs3
{
	union alignas(16) hypervisor_context_t
	{
		u64 regs[16];

		struct
		{
			u64 pc;
			u64 sp;

			u64 x18;
			u64 x19;
			u64 x20;
			u64 x21;
			u64 x22;
			u64 x23;
			u64 x24;
			u64 x25;
			u64 x26;
			u64 x27;
			u64 x28;
			u64 x29;
			u64 x30;

			// x0-x17 unused
		} aarch64;

		struct
		{
			u64 sp;

			// Other regs unused
		} x86;
	};
}
