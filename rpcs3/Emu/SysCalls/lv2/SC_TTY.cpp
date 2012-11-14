#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"

int SysCalls::lv2TtyRead(PPUThread& CPU)
{
	const u32 ch = CPU.GPR[3];
	const u64 buf_addr = CPU.GPR[4];
	const u32 len = CPU.GPR[5];
	const u64 preadlen_addr = CPU.GPR[6];
	ConLog.Warning("lv2TtyRead: ch: %d, buf addr: %llx, len: %d", ch, buf_addr, len);
	Memory.Write32NN(preadlen_addr, len);

	return CELL_OK;
}

int SysCalls::lv2TtyWrite(PPUThread& CPU)
{
	const u32 ch = CPU.GPR[3];
	const u64 buf_addr = CPU.GPR[4];
	const u32 len = CPU.GPR[5];
	const u64 pwritelen_addr = CPU.GPR[6];
	//for(uint i=0; i<32; ++i) ConLog.Write("r%d = 0x%llx", i, CPU.GPR[i]);
	//ConLog.Warning("lv2TtyWrite: ch: %d, buf addr: %llx, len: %d", ch, buf_addr, len);
	if(ch < 0 || ch > 15 || len <= 0) return CELL_EINVAL;
	if(!Memory.IsGoodAddr(buf_addr)) return CELL_EINVAL;
	
	Emu.GetDbgCon().Write(ch, Memory.ReadString(buf_addr, len));
	
	if(!Memory.IsGoodAddr(pwritelen_addr)) return CELL_EFAULT;

	Memory.Write32(pwritelen_addr, len);

	return CELL_OK;
}