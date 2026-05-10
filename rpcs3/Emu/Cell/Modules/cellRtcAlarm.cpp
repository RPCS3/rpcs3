#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellRtcAlarm);

error_code cellRtcAlarmRegister()
{
	UNIMPLEMENTED_FUNC(cellRtcAlarm);
	return CELL_OK;
}

error_code cellRtcAlarmUnregister()
{
	UNIMPLEMENTED_FUNC(cellRtcAlarm);
	return CELL_OK;
}

error_code cellRtcAlarmNotification()
{
	UNIMPLEMENTED_FUNC(cellRtcAlarm);
	return CELL_OK;
}

error_code cellRtcAlarmStopRunning()
{
	UNIMPLEMENTED_FUNC(cellRtcAlarm);
	return CELL_OK;
}

error_code cellRtcAlarmGetStatus()
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
