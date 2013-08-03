#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void sys_fs_init();
Module sys_fs(0x000e, sys_fs_init);

void sys_fs_init()
{
	sys_fs.AddFunc(0x718bf5f8, cellFsOpen);
	sys_fs.AddFunc(0x4d5ff8e2, cellFsRead);
	sys_fs.AddFunc(0xecdcf2ab, cellFsWrite);
	sys_fs.AddFunc(0x2cb51f0d, cellFsClose);
	sys_fs.AddFunc(0x3f61245c, cellFsOpendir);
	sys_fs.AddFunc(0x5c74903d, cellFsReaddir);
	sys_fs.AddFunc(0xff42dcc3, cellFsClosedir);
	sys_fs.AddFunc(0x7de6dced, cellFsStat);
	sys_fs.AddFunc(0xef3efa34, cellFsFstat);
	sys_fs.AddFunc(0xba901fe6, cellFsMkdir);
	sys_fs.AddFunc(0xf12eecc8, cellFsRename);
	sys_fs.AddFunc(0x2796fdf3, cellFsRmdir);
	sys_fs.AddFunc(0x7f4677a8, cellFsUnlink);
	sys_fs.AddFunc(0xa397d042, cellFsLseek);
}
