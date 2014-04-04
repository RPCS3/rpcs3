#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

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
	cellSheap.AddFunc(0xbbb47cd8, cellSheapInitialize);
	cellSheap.AddFunc(0x4b1383fb, cellSheapAllocate);
	cellSheap.AddFunc(0x5c5994bd, cellSheapFree);
	cellSheap.AddFunc(0x37968718, cellSheapQueryMax);
	cellSheap.AddFunc(0x7fa23275, cellSheapQueryFree);

	// (TODO: Some cellKeySheap* functions are missing)
	cellSheap.AddFunc(0xa1b25841, cellKeySheapInitialize);
	cellSheap.AddFunc(0x4a5b9659, cellKeySheapBufferNew);
	cellSheap.AddFunc(0xe6b37362, cellKeySheapBufferDelete);

	cellSheap.AddFunc(0x3478e1e6, cellKeySheapMutexNew);
	cellSheap.AddFunc(0x2452679f, cellKeySheapMutexDelete);
	cellSheap.AddFunc(0xe897c835, cellKeySheapBarrierNew);
	cellSheap.AddFunc(0xf6f5fbca, cellKeySheapBarrierDelete);
	cellSheap.AddFunc(0x69a5861d, cellKeySheapSemaphoreNew);
	cellSheap.AddFunc(0x73a45cf8, cellKeySheapSemaphoreDelete);
	cellSheap.AddFunc(0xf01ac471, cellKeySheapRwmNew);
	cellSheap.AddFunc(0xed136702, cellKeySheapRwmDelete);
	cellSheap.AddFunc(0x987e260e, cellKeySheapQueueNew);
	cellSheap.AddFunc(0x79a6abd0, cellKeySheapQueueDelete);
}