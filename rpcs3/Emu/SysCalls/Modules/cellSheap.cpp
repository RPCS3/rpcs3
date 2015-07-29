#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellSheap;

// Return Codes
enum
{
	CELL_SHEAP_ERROR_INVAL    = 0x80410302,
	CELL_SHEAP_ERROR_BUSY     = 0x8041030A,
	CELL_SHEAP_ERROR_ALIGN    = 0x80410310,
	CELL_SHEAP_ERROR_SHORTAGE = 0x80410312,
};

s32 cellSheapInitialize()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellSheapAllocate()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellSheapFree()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellSheapQueryMax()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellSheapQueryFree()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellKeySheapInitialize()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellKeySheapBufferNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellKeySheapBufferDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellKeySheapMutexNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellKeySheapMutexDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellKeySheapBarrierNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellKeySheapBarrierDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellKeySheapSemaphoreNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellKeySheapSemaphoreDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellKeySheapRwmNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellKeySheapRwmDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellKeySheapQueueNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

s32 cellKeySheapQueueDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

Module cellSheap("cellSheap", []()
{
	REG_FUNC(cellSheap, cellSheapInitialize);
	REG_FUNC(cellSheap, cellSheapAllocate);
	REG_FUNC(cellSheap, cellSheapFree);
	REG_FUNC(cellSheap, cellSheapQueryMax);
	REG_FUNC(cellSheap, cellSheapQueryFree);

	REG_FUNC(cellSheap, cellKeySheapInitialize);
	REG_FUNC(cellSheap, cellKeySheapBufferNew);
	REG_FUNC(cellSheap, cellKeySheapBufferDelete);

	REG_FUNC(cellSheap, cellKeySheapMutexNew);
	REG_FUNC(cellSheap, cellKeySheapMutexDelete);
	REG_FUNC(cellSheap, cellKeySheapBarrierNew);
	REG_FUNC(cellSheap, cellKeySheapBarrierDelete);
	REG_FUNC(cellSheap, cellKeySheapSemaphoreNew);
	REG_FUNC(cellSheap, cellKeySheapSemaphoreDelete);
	REG_FUNC(cellSheap, cellKeySheapRwmNew);
	REG_FUNC(cellSheap, cellKeySheapRwmDelete);
	REG_FUNC(cellSheap, cellKeySheapQueueNew);
	REG_FUNC(cellSheap, cellKeySheapQueueDelete);
});
