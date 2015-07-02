#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/System.h"
#include "LogBase.h"

bool LogBase::CheckLogging() const
{
	return Ini.HLELogging.GetValue() || m_logging;
}

void LogBase::LogOutput(LogType type, const std::string& text) const
{
	switch (type)
	{
	case LogNotice: LOG_NOTICE(HLE, GetName() + ": " + text); break;
	case LogSuccess: LOG_SUCCESS(HLE, GetName() + ": " + text); break;
	case LogWarning: LOG_WARNING(HLE, GetName() + ": " + text); break;
	case LogError: LOG_ERROR(HLE, GetName() + " error: " + text); break;
	case LogTodo: LOG_ERROR(HLE, GetName() + " TODO: " + text); break;
	case LogFatal: throw EXCEPTION("%s error: %s", GetName().c_str(), text.c_str());
	}
}
