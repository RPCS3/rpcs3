#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellRtcAlarm);

s32 cellRtcAlarmRegister()
{
	UNIMPLEMENTED_FUNC(cellRtcAlarm);
	return CELL_OK;
}

s32 cellRtcAlarmUnregister()
{
	UNIMPLEMENTED_FUNC(cellRtcAlarm);
	return CELL_OK;
}

s32 cellRtcAlarmGetStatus()
{
	UNIMPLEMENTED_FUNC(cellRtcAlarm);
	return CELL_OK;
}

s32 cellRtcAlarm_AD8D9839()
{
	UNIMPLEMENTED_FUNC(cellRtcAlarm);
	return CELL_OK;
}

s32 cellRtcAlarm_B287748C()
{
	UNIMPLEMENTED_FUNC(cellRtcAlarm);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellRtcAlarm)("cellRtcAlarm", []()
{
	REG_FUNC(cellRtcAlarm, cellRtcAlarmRegister);
	REG_FUNC(cellRtcAlarm, cellRtcAlarmUnregister);
	REG_FUNC(cellRtcAlarm, cellRtcAlarmGetStatus);

	REG_FNID(cellRtcAlarm, 0xAD8D9839, cellRtcAlarm_AD8D9839);
	REG_FNID(cellRtcAlarm, 0xB287748C, cellRtcAlarm_B287748C);
});
