#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

Module sys_fs("sys_fs", 0x000e);

struct _sys_fs_init
{
	_sys_fs_init()
	{
		sys_fs.AddFunc(0x718bf5f8, bind_func(cellFsOpen));
		sys_fs.AddFunc(0x4d5ff8e2, bind_func(cellFsRead));
		sys_fs.AddFunc(0xecdcf2ab, bind_func(cellFsWrite));
		sys_fs.AddFunc(0x2cb51f0d, bind_func(cellFsClose));
		sys_fs.AddFunc(0x3f61245c, bind_func(cellFsOpendir));
		sys_fs.AddFunc(0x5c74903d, bind_func(cellFsReaddir));
		sys_fs.AddFunc(0xff42dcc3, bind_func(cellFsClosedir));
		sys_fs.AddFunc(0x7de6dced, bind_func(cellFsStat));
		sys_fs.AddFunc(0xef3efa34, bind_func(cellFsFstat));
		sys_fs.AddFunc(0xba901fe6, bind_func(cellFsMkdir));
		sys_fs.AddFunc(0xf12eecc8, bind_func(cellFsRename));
		sys_fs.AddFunc(0x2796fdf3, bind_func(cellFsRmdir));
		sys_fs.AddFunc(0x7f4677a8, bind_func(cellFsUnlink));
		sys_fs.AddFunc(0xa397d042, bind_func(cellFsLseek));
	}
} sys_fs_init;