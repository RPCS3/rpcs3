#include <string>
#include "Utilities/StrFmt.h"

namespace cereal
{
	[[noreturn]] void throw_exception(const std::string& err)
	{
		fmt::throw_exception("%s", err);
	}
}
