#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellSheap);

// Return Codes
enum CellSheapError : u32
{
	CELL_SHEAP_ERROR_INVAL    = 0x80410302,
	CELL_SHEAP_ERROR_BUSY     = 0x8041030A,
	CELL_SHEAP_ERROR_ALIGN    = 0x80410310,
	CELL_SHEAP_ERROR_SHORTAGE = 0x80410312,
};

template <>
void fmt_class_string<CellSheapError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_SHEAP_ERROR_INVAL);
			STR_CASE(CELL_SHEAP_ERROR_BUSY);
			STR_CASE(CELL_SHEAP_ERROR_ALIGN);
			STR_CASE(CELL_SHEAP_ERROR_SHORTAGE);
		}

		return unknown;
	});
}

error_code cellSheapInitialize()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellSheapAllocate()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellSheapFree()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellSheapQueryMax()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellSheapQueryFree()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellKeySheapInitialize()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellKeySheapBufferNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellKeySheapBufferDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellKeySheapMutexNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellKeySheapMutexDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellKeySheapBarrierNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellKeySheapBarrierDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellKeySheapSemaphoreNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellKeySheapSemaphoreDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellKeySheapRwmNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellKeySheapRwmDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellKeySheapQueueNew()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

error_code cellKeySheapQueueDelete()
{
	UNIMPLEMENTED_FUNC(cellSheap);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSheap)("cellSheap", []()
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
