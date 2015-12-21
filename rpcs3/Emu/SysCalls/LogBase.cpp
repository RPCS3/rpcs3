#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/state.h"
#include "LogBase.h"

bool LogBase::CheckLogging() const
{
	return rpcs3::config.misc.log.hle_logging.value() || m_logging;
}

void LogBase::LogOutput(LogType type, std::string text) const
{
	switch (type)
	{
	case LogNotice: LOG_NOTICE(HLE, GetName() + ": " + text); break;
	case LogSuccess: LOG_SUCCESS(HLE, GetName() + ": " + text); break;
	case LogWarning: LOG_WARNING(HLE, GetName() + ": " + text); break;
	case LogError: LOG_ERROR(HLE, GetName() + ": " + text); break;
	case LogTodo: LOG_ERROR(HLE, GetName() + " TODO: " + text); break;
	}
}
