#include <string>

#include <rpcs3/core/platform.h>
#include <rpcs3/core/log.h>
#include "log_base.h"

namespace rpcs3::syscalls
{
	bool log_base::check_logging() const
	{
		return /*rpcs3::config.misc.log.hle_logging.value() ||*/ m_logging;
	}

	void log_base::log_output(log_type type, std::string text) const
	{
		switch (type)
		{
		case log_type::notice: LOG_NOTICE(HLE, name() + ": " + text); break;
		case log_type::success: LOG_SUCCESS(HLE, name() + ": " + text); break;
		case log_type::warning: LOG_WARNING(HLE, name() + ": " + text); break;
		case log_type::error: LOG_ERROR(HLE, name() + ": " + text); break;
		case log_type::todo: LOG_ERROR(HLE, name() + " TODO: " + text); break;
		}
	}
}