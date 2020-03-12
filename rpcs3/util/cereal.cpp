#include <string>
#include "Utilities/Thread.h"

namespace cereal
{
	[[noreturn]] void throw_exception(const std::string& err)
	{
		report_fatal_error(err);
	}
}
