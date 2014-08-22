#include "stdafx.h"
#include "Utilities/Log.h"
#include "LogBase.h"

void LogBase::LogNotice(const std::string& text)
{
	LOG_NOTICE(HLE, "%s", text.c_str());
}

void LogBase::LogWarning(const std::string& text)
{
	LOG_WARNING(HLE, "%s", text.c_str());
}

void LogBase::LogError(const std::string& text)
{
	LOG_ERROR(HLE, "%s", text.c_str());
}