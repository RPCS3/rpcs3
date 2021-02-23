#include "stdafx.h"
#include "Emu/system_config.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"

#include "sys_tty.h"

#include <deque>

LOG_CHANNEL(sys_tty);

extern std::array<std::deque<std::string>, 16> g_tty_input;

tty_t::tty_t()
{
	const auto tty_path = fs::get_cache_dir() + "TTY.log";
	fd.open(tty_path, fs::write + fs::read + fs::create);

	if (!fd)
	{
		sys_tty.error("Failed to create TTY log: %s (%s)", tty_path, fs::g_tls_error);
	}
	else
	{
		m_start_pos = fd.seek(0, fs::seek_end);
	}
}

void tty_t::write_header() const
{
	// Either title ID or name of the file
	const std::string name = Emu.GetTitleID().empty() ? Emu.argv[0].substr(Emu.argv[0].find_last_of(fs::delim) + 1) : Emu.GetTitleID();

	char last = '\n';

	if (m_start_pos)
	{
		// Get last character
		fd.seek(m_start_pos - 1);
		fd.read(last);
	}
	
	fd.write(fmt::format("%s[RPCS3] TTY of %s:\n", last != '\n' ? "\n" : "", name));
}

tty_t::~tty_t()
{
	std::string str;

	if (fd)
	{
		fd.seek(m_start_pos);
		fd.read(str, fd.size() - m_start_pos);
	}

	// Print TTY of current application
	sys_tty.notice("\n%s", str);
}

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
		const auto tty = g_fxo->get<tty_t>();

		std::lock_guard lock(tty->mtx);

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

error_code sys_tty_write(s32 ch, vm::cptr<char> buf, u32 len, vm::ptr<u32> pwritelen)
{
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
			sys_tty.notice(u8"sys_tty_write(): “%s”", msg);

			const auto tty = g_fxo->get<tty_t>();

			if (tty->fd)
			{
				reader_lock lock(tty->mtx);

				if (tty->is_first.try_inc(1))
				{
					// First thread to write to TTY, write header
					tty->write_header();
					tty->is_first = 2;
					tty->is_first.notify_all();
				}
				else if (tty->is_first == 1u)
				{
					// Wait for header to be written
					tty->is_first.wait(1);
				}

				tty->fd.write(msg);
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
