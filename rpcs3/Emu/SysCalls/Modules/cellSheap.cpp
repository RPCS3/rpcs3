#include "stdafx.h"
#if 0

void cellSheap_init();
Module cellSheap(0x000c, cellSheap_init);

// Return Codes
enum
{
	CELL_SHEAP_ERROR_INVAL    = 0x80410302,
	CELL_SHEAP_ERROR_BUSY     = 0x8041030A,
	CELL_SHEAP_ERROR_ALIGN    = 0x80410310,
	CELL_SHEAP_ERROR_SHORTAGE = 0x80410312,
};

int cellSheapInitialize()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellSheapAllocate()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellSheapFree()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellSheapQueryMax()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellSheapQueryFree()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellKeySheapInitialize()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellKeySheapBufferNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellKeySheapBufferDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellKeySheapMutexNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellKeySheapMutexDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellKeySheapBarrierNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellKeySheapBarrierDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellKeySheapSemaphoreNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellKeySheapSemaphoreDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellKeySheapRwmNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellKeySheapRwmDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellKeySheapQueueNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

int cellKeySheapQueueDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

void cellSheap_init()
{
	REG_FUNC(cellSheap, cellSheapInitialize);
	REG_FUNC(cellSheap, cellSheapAllocate);
	REG_FUNC(cellSheap, cellSheapFree);
	REG_FUNC(cellSheap, cellSheapQueryMax);
	REG_FUNC(cellSheap, cellSheapQueryFree);

	// (TODO: Some cellKeySheap* functions are missing)
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
}
#endif
