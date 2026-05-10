#pragma once

#include <string_view>

namespace utils
{
	enum console_stream
	{
		std_out = 0x01,
		std_err = 0x02,
		std_in  = 0x04,
	};

	void attach_console(int stream, bool open_console);

	void output_stderr(std::string_view str, bool with_endline = false);
}
