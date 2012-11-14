#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"

SysCallBase sc_p("Process");

int sys_process_getpid()
{
	return 1;
}

int sys_game_process_exitspawn(	u32 path_addr, u32 argv_addr, u32 envp_addr,
								u32 data, u32 data_size, int prio, u64 flags )
{
	sc_p.Log("sys_game_process_exitspawn: ");
	sc_p.Log("path: %s", Memory.ReadString(path_addr));
	sc_p.Log("argv: %x", Memory.Read32(argv_addr));
	sc_p.Log("envp: %x", Memory.Read32(envp_addr));
	sc_p.Log("data: %x", data);
	sc_p.Log("data_size: %x", data_size);
	sc_p.Log("prio: %d", prio);
	sc_p.Log("flags: %d", flags);
	return CELL_OK;
}

int SysCalls::lv2ProcessGetPid(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessGetPid");
	Memory.Write32(CPU.GPR[4], CPU.GetId());
	return CELL_OK;
}
int SysCalls::lv2ProcessWaitForChild(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessWaitForChild");
	return CELL_OK;
}
int SysCalls::lv2ProcessGetStatus(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessGetStatus");
	if(CPU.IsSPU()) return CELL_UNKNOWN_ERROR;
	//Memory.Write32(CPU.GPR[4], GetPPUThreadStatus(CPU));
	return CELL_OK;
}
int SysCalls::lv2ProcessDetachChild(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessDetachChild");
	return CELL_OK;
}
int SysCalls::lv2ProcessGetNumberOfObject(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessGetNumberOfObject");
	Memory.Write32(CPU.GPR[4], 1);
	return CELL_OK;
}
int SysCalls::lv2ProcessGetId(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessGetId");
	Memory.Write32(CPU.GPR[4], CPU.GetId());
	return CELL_OK;
}
int SysCalls::lv2ProcessGetPpid(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessGetPpid");
	Memory.Write32(CPU.GPR[4], CPU.GetId());
	return CELL_OK;
}
int SysCalls::lv2ProcessKill(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessKill[pid: 0x%llx]", CPU.GPR[3]);
	CPU.Close();
	return CELL_OK;
}
int SysCalls::lv2ProcessExit(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessExit(%lld)", CPU.GPR[3]);
	Emu.Pause();
	return CELL_OK;
}
int SysCalls::lv2ProcessWaitForChild2(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessWaitForChild2[r3: 0x%llx, r4: 0x%llx, r5: 0x%llx, r6: 0x%llx, r7: 0x%llx, r8: 0x%llx]",
		CPU.GPR[3], CPU.GPR[4], CPU.GPR[5], CPU.GPR[6], CPU.GPR[7], CPU.GPR[8]);
	return CELL_OK;
}
int SysCalls::lv2ProcessGetSdkVersion(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessGetSdkVersion[r3: 0x%llx, r4: 0x%llx]", CPU.GPR[3], CPU.GPR[4]);
	CPU.GPR[4] = 0x360001; //TODO
	return CELL_OK;
}