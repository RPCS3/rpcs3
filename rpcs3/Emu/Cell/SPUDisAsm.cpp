#include "stdafx.h"
#include "SPUDisAsm.h"

constexpr spu_decoder<SPUDisAsm> s_spu_disasm;

u32 SPUDisAsm::disasm(u32 pc)
{
	const u32 op = *reinterpret_cast<const be_t<u32>*>(offset + pc);
	(this->*(s_spu_disasm.decode(op)))({ op });
	return 4;
}
