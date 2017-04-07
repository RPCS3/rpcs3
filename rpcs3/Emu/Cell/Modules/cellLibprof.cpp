#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
 
 logs::channel cellLibprof("cellLibprof", logs::level::notice);
 
 s32 cellLibprof_05893E7C() 
 {
 	cellLibprof.todo("cellLibprof_05893E7C");
 	return CELL_OK;
 }

s32 cellLibprof_6D045C2E() 
 {
 	cellLibprof.todo("cellLibprof_6D045C2E");
 	return CELL_OK;
 }

 DECLARE(ppu_module_manager::cellLibprof)("cellLibprof", []()
 {
 	  REG_FNID(cellLibprof, 0x05893E7C, cellLibprof_05893E7C);
	  REG_FNID(cellLibprof, 0x6D045C2E, cellLibprof_6D045C2E);
 });
