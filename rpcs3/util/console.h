#pragma once

namespace utils
{
	enum console_stream
	{
		std_out = 0x01,
		std_err = 0x02,
		std_in  = 0x04,
	};

	void attach_console(int stream, bool open_console);
}
