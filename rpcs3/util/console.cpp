#include "console.h"

#ifdef _WIN32
#include "Windows.h"
#include <stdio.h>
#endif

namespace utils
{
	void attach_console([[maybe_unused]] int stream, [[maybe_unused]] bool open_console)
	{
#ifdef _WIN32
		if (!stream)
		{
			return;
		}

		if (!(AttachConsole(ATTACH_PARENT_PROCESS) || (open_console && AllocConsole())))
		{
			return;
		}

		if (stream & console_stream::std_out)
		{
			[[maybe_unused]] const auto con_out = freopen("CONOUT$", "w", stdout);
		}

		if (stream & console_stream::std_err)
		{
			[[maybe_unused]] const auto con_err = freopen("CONOUT$", "w", stderr);
		}

		if (stream & console_stream::std_in)
		{
			[[maybe_unused]] const auto con_in = freopen("CONIN$", "r", stdin);
		}
#endif
	}
}
