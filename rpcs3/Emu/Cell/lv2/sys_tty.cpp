#include "stdafx.h"
#include "Emu/system_config.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/timers.hpp"

#include "sys_tty.h"

#include <deque>
#include <mutex>

LOG_CHANNEL(sys_tty);

extern fs::file g_tty;
extern atomic_t<s64> g_tty_size;
extern std::array<std::deque<std::string>, 16> g_tty_input;
extern std::mutex g_tty_mutex;

error_code sys_tty_read(s32 ch, vm::ptr<char> buf, u32 len, vm::ptr<u32> preadlen)
{
	sys_tty.trace("sys_tty_read(ch=%d, buf=*0x%x, len=%d, preadlen=*0x%x)", ch, buf, len, preadlen);

	if (!g_cfg.core.debug_console_mode)
	{
		return CELL_EIO;
	}

	if (ch > 15 || ch < 0 || !buf)
	{
		return CELL_EINVAL;
	}

	if (ch < SYS_TTYP_USER1)
	{
		sys_tty.warning("sys_tty_read called with system channel %d", ch);
	}

	usz chars_to_read = 0; // number of chars that will be read from the input string
	std::string tty_read;     // string for storage of read chars

	if (len > 0)
	{
		std::lock_guard lock(g_tty_mutex);

		if (!g_tty_input[ch].empty())
		{
			// reference to our first queue element
			std::string& input = g_tty_input[ch].front();

			// we have to stop reading at either a new line, the param len, or our input string size
			usz new_line_pos = input.find_first_of('\n');

			if (new_line_pos != input.npos)
			{
				chars_to_read = std::min(new_line_pos, static_cast<usz>(len));
			}
			else
			{
				chars_to_read = std::min(input.size(), static_cast<usz>(len));
			}

			// read the previously calculated number of chars from the beginning of the input string
			tty_read = input.substr(0, chars_to_read);

			// remove the just read text from the input string
			input = input.substr(chars_to_read, input.size() - 1);

			if (input.empty())
			{
				// pop the first queue element if it was completely consumed
				g_tty_input[ch].pop_front();
			}
		}
	}

	if (!preadlen)
	{
		return CELL_EFAULT;
	}

	*preadlen = static_cast<u32>(chars_to_read);

	if (chars_to_read > 0)
	{
		std::memcpy(buf.get_ptr(), tty_read.c_str(), chars_to_read);
		sys_tty.success("sys_tty_read(ch=%d, len=%d) read %s with length %d", ch, len, tty_read, *preadlen);
	}

	return CELL_OK;
}

std::string dump_useful_thread_info();

error_code sys_tty_write([[maybe_unused]] ppu_thread& ppu, s32 ch, vm::cptr<char> buf, u32 len, vm::ptr<u32> pwritelen)
{
	ppu.state += cpu_flag::wait;

	sys_tty.notice("sys_tty_write(ch=%d, buf=*0x%x, len=%d, pwritelen=*0x%x)", ch, buf, len, pwritelen);

	std::string msg;

	if (static_cast<s32>(len) > 0 && vm::check_addr(buf.addr(), vm::page_readable, len))
	{
		msg.resize(len);

		if (!vm::try_access(buf.addr(), msg.data(), len, false))
		{
			msg.clear();
		}
	}

	auto find_word = [](std::string_view msg, std::string_view word) -> bool
	{
		// Match uppercase and lowercase starting words
		const usz index = msg.find(word.substr(1));

		if (index != umax && index >= 1u)
		{
			return std::tolower(static_cast<u8>(msg[index - 1])) == word[0];
		}

		return false;
	};

	std::string_view sample = std::string_view(msg).substr(0, 1024);

	const bool warning = find_word(sample, "failed"sv) || find_word(sample, "abort"sv) || find_word(sample, "crash"sv)
		|| find_word(sample, "error"sv) || find_word(sample, "unexpected"sv) || find_word(sample, "0x8001"sv);

	sample = {}; // Remove reference to string

	if (msg.size() >= 2u && [&]()
	{
		static thread_local u64 last_write = 0;

		// Dump thread about every period which TTY was not being touched for about half a second
		const u64 current = get_system_time();
		return current - std::exchange(last_write, current) >= (warning ? 500'000 : 3'000'000);
	}())
	{
		ppu_log.notice("\n%s", dump_useful_thread_info());
	}

	// Hack: write to tty even on CEX mode, but disable all error checks
	if (ch < 0 || ch > 15)
	{
		if (g_cfg.core.debug_console_mode)
		{
			return CELL_EINVAL;
		}
		else
		{
			msg.clear();
		}
	}

	if (g_cfg.core.debug_console_mode)
	{
		// Don't modify it in CEX mode
		len = static_cast<s32>(len) > 0 ? len : 0;
	}

	if (static_cast<s32>(len) > 0)
	{
		if (!msg.empty())
		{
			if (msg.ends_with("\n"))
			{
				// Avoid logging trailing newlines, log them verbosely instead
				const std::string_view msg_clear = std::string_view(msg).substr(0, msg.find_last_not_of('\n') + 1);

				if (msg.size() - 1 == msg_clear.size())
				{
					(warning ? sys_tty.warning : sys_tty.notice)(u8"sys_tty_write(): “%s“ << endl", msg_clear);
				}
				else
				{
					(warning ? sys_tty.warning : sys_tty.notice)(u8"sys_tty_write(): “%s” << endl(%u)", msg_clear, msg.size() - msg_clear.size());
				}
			}
			else
			{
				(warning ? sys_tty.warning : sys_tty.notice)(u8"sys_tty_write(): “%s”", msg);
			}

			if (g_tty)
			{
				// Lock size by making it negative
				g_tty_size -= (1ll << 48);
				g_tty.write(msg);
				g_tty_size += (1ll << 48) + len;
			}
		}
		else if (g_cfg.core.debug_console_mode)
		{
			return {CELL_EFAULT, buf.addr()};
		}
	}

	if (!pwritelen.try_write(len))
	{
		return {CELL_EFAULT, pwritelen};
	}

	return CELL_OK;
}
