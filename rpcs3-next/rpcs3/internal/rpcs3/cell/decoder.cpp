#include <rpcs3/vm/vm.h>
#include "decoder.h"

namespace common
{
	namespace cell
	{
		u32 decoder::decode_memory(const u32 address)
		{
			u32 instr = vm::ps3::read32(address);
			Decode(instr);

			return sizeof(u32);
		}
	}
}