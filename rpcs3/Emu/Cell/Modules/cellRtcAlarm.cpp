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

s32 cellRtcAlarmNotification()
{
	UNIMPLEMENTED_FUNC(cellRtcAlarm);
	return CELL_OK;
}

s32 cellRtcAlarmStopRunning()
{
	UNIMPLEMENTED_FUNC(cellRtcAlarm);
	return CELL_OK;
}

s32 cellRtcAlarmGetStatus()
{
	UNIMPLEMENTED_FUNC(cellRtcAlarm);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellRtcAlarm)("cellRtcAlarm", []()
{
	REG_FUNC(cellRtcAlarm, cellRtcAlarmRegister);
	REG_FUNC(cellRtcAlarm, cellRtcAlarmUnregister);
	REG_FUNC(cellRtcAlarm, cellRtcAlarmNotification);
	REG_FUNC(cellRtcAlarm, cellRtcAlarmStopRunning);
	REG_FUNC(cellRtcAlarm, cellRtcAlarmGetStatus);
});
