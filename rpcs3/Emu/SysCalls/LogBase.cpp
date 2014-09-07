#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/System.h"
#include "LogBase.h"

bool LogBase::CheckLogging() const
{
	return Ini.HLELogging.GetValue();
}

void LogBase::LogOutput(LogType type, const char* info, const std::string& text)
{
	switch (type)
	{
	case LogNotice: LOG_NOTICE(HLE, "%s%s%s", GetName().c_str(), info, text.c_str()); break;
	case LogSuccess: LOG_SUCCESS(HLE, "%s%s%s", GetName().c_str(), info, text.c_str()); break;
	case LogWarning: LOG_WARNING(HLE, "%s%s%s", GetName().c_str(), info, text.c_str()); break;
	case LogError: LOG_ERROR(HLE, "%s%s%s", GetName().c_str(), info, text.c_str()); break;
	}
}

void LogBase::LogOutput(LogType type, const u32 id, const char* info, const std::string& text)
{
	switch (type)
	{
	case LogNotice: LOG_NOTICE(HLE, "%s[%d]%s%s", GetName().c_str(), id, info, text.c_str()); break;
	case LogSuccess: LOG_SUCCESS(HLE, "%s[%d]%s%s", GetName().c_str(), id, info, text.c_str()); break;
	case LogWarning: LOG_WARNING(HLE, "%s[%d]%s%s", GetName().c_str(), id, info, text.c_str()); break;
	case LogError: LOG_ERROR(HLE, "%s[%d]%s%s", GetName().c_str(), id, info, text.c_str()); break;
	}
}