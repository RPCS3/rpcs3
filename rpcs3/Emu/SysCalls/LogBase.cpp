#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/System.h"
#include "LogBase.h"

bool LogBase::CheckLogging() const
{
	return Ini.HLELogging.GetValue() || m_logging;
}

void LogBase::LogOutput(LogType type, const char* info, const std::string& text) const
{
	switch (type)
	{
	case LogNotice: LOG_NOTICE(HLE, "%s%s%s", GetName().c_str(), info, text.c_str()); break;
	case LogSuccess: LOG_SUCCESS(HLE, "%s%s%s", GetName().c_str(), info, text.c_str()); break;
	case LogWarning: LOG_WARNING(HLE, "%s%s%s", GetName().c_str(), info, text.c_str()); break;
	case LogError: LOG_ERROR(HLE, "%s%s%s", GetName().c_str(), info, text.c_str()); break;
	}
}

void LogBase::LogOutput(LogType type, const u32 id, const char* info, const std::string& text) const
{
	switch (type)
	{
	case LogNotice: LOG_NOTICE(HLE, "%s[%d]%s%s", GetName().c_str(), id, info, text.c_str()); break;
	case LogSuccess: LOG_SUCCESS(HLE, "%s[%d]%s%s", GetName().c_str(), id, info, text.c_str()); break;
	case LogWarning: LOG_WARNING(HLE, "%s[%d]%s%s", GetName().c_str(), id, info, text.c_str()); break;
	case LogError: LOG_ERROR(HLE, "%s[%d]%s%s", GetName().c_str(), id, info, text.c_str()); break;
	}
}

hle::error::error(s32 errorCode, const char* errorText)
	: code(errorCode)
	, base(nullptr)
	, text(errorText ? errorText : "")
{
}

hle::error::error(s32 errorCode, const LogBase& errorBase, const char* errorText)
	: code(errorCode)
	, base(&errorBase)
	, text(errorText ? errorText : "")
{
}

hle::error::error(s32 errorCode, const LogBase* errorBase, const char* errorText)
	: code(errorCode)
	, base(errorBase)
	, text(errorText ? errorText : "")
{
}

void hle::error::print(const char* func)
{
	if (!text.empty())
	{
		if (base)
		{
			if (func)
			{
				base->Error("%s(): %s (0x%x)", func, text.c_str(), code);
			}
			else
			{
				base->Error("%s (0x%x)", text.c_str(), code);
			}
		}
		else
		{
			if (func)
			{
				LOG_ERROR(HLE, "%s(): %s (0x%x)", func, text.c_str(), code);
			}
			else
			{
				LOG_ERROR(HLE, "%s (0x%x)", text.c_str(), code);
			}
		}
	}
}
