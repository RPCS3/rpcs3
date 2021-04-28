#include "memory_string_searcher.h"
#include "Emu/Memory/vm.h"
#include "Emu/Memory/vm_reservation.h"

#include "Utilities/Thread.h"
#include "Utilities/StrUtil.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>

#include <charconv>

#include "util/logs.hpp"
#include "util/sysinfo.hpp"
#include "util/asm.hpp"

LOG_CHANNEL(gui_log, "GUI");

enum : int
{
	as_string,
	as_hex,
	as_f64,
	as_f32,
};

memory_string_searcher::memory_string_searcher(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("String Searcher"));
	setObjectName("memory_string_searcher");
	setAttribute(Qt::WA_DeleteOnClose);

	m_addr_line = new QLineEdit(this);
	m_addr_line->setFixedWidth(QLabel("This is the very length of the lineedit due to hidpi reasons.").sizeHint().width());
	m_addr_line->setPlaceholderText(tr("Search..."));
	m_addr_line->setMaxLength(4096);

	QPushButton* button_search = new QPushButton(tr("&Search"), this);

	m_chkbox_case_insensitive = new QCheckBox(tr("Case Insensitive"), this);
	m_chkbox_case_insensitive->setCheckable(true);
	m_chkbox_case_insensitive->setToolTip(tr("When using string mode, the characters' case will not matter both in string and in memory."
		"\nWarning: this may reduce performance of the search."));

	m_cbox_input_mode = new QComboBox(this);
	m_cbox_input_mode->addItem("String", QVariant::fromValue(+as_string));
	m_cbox_input_mode->addItem("HEX bytes/integer", QVariant::fromValue(+as_hex));
	m_cbox_input_mode->addItem("Double", QVariant::fromValue(+as_f64));
	m_cbox_input_mode->addItem("Float", QVariant::fromValue(+as_f32));
	m_cbox_input_mode->setToolTip(tr("String: search the memory for the specified string."
		"\nHEX bytes/integer: search the memory for hexadecimal values. Spaces, commas, \"0x\", \"0X\", \"\\x\" ensure separation of bytes but they are not mandatory."
		"\nDouble: reinterpret the string as 64-bit precision floating point value. Values are searched for exact representation, meaning -0 != 0."
		"\nFloat: reinterpret the string as 32-bit precision floating point value. Values are searched for exact representation, meaning -0 != 0."));

	QHBoxLayout* hbox_panel = new QHBoxLayout();
	hbox_panel->addWidget(m_addr_line);
	hbox_panel->addWidget(m_cbox_input_mode);
	hbox_panel->addWidget(m_chkbox_case_insensitive);
	hbox_panel->addWidget(button_search);

	setLayout(hbox_panel);

	connect(button_search, &QAbstractButton::clicked, this, &memory_string_searcher::OnSearch);

	layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void memory_string_searcher::OnSearch()
{
	std::string wstr = m_addr_line->text().toStdString();

	if (wstr.empty() || wstr.size() >= 4096u)
	{
		gui_log.error("String is empty or too long (size=%u)", wstr.size());
		return;
	}

	gui_log.notice("Searching for %s", wstr);

	const int mode = std::max(m_cbox_input_mode->currentIndex(), 0);
	bool case_insensitive = false;

	// First characters for case insensitive search
	const char first_chars[2]{ static_cast<char>(::tolower(wstr[0])), static_cast<char>(::toupper(wstr[0])) };
	std::string_view insensitive_search{first_chars, 2};

	if (insensitive_search[0] == insensitive_search[1])
	{
		// Optimization
		insensitive_search.remove_suffix(1);
	}

	switch (mode)
	{
	case as_string:
	{
		case_insensitive = m_chkbox_case_insensitive->isChecked();

		if (case_insensitive)
		{
			std::transform(wstr.begin(), wstr.end(), wstr.begin(), ::tolower);
		}

		break;
	}
	case as_hex:
	{
		constexpr std::string_view hex_chars = "0123456789ABCDEFabcdef";

		// Split
		std::vector<std::string> parts = fmt::split(wstr, {" ", ",", "0x", "0X", "\\x"});

		// Pad zeroes
		for (std::string& part : parts)
		{
			if (part.size() % 2)
			{
				gui_log.warning("Padding string part with '0' at front due to odd hexadeciaml characters count.");
				part.insert(part.begin(), '0');
			}
		}

		// Concat strings
		wstr.clear();
		for (const std::string& part : parts)
		{
			wstr += part;
		}

		if (const usz pos = wstr.find_first_not_of(hex_chars); pos != umax)
		{
			gui_log.error("String '%s' cannot be interpreted as hexadecimal byte string due to unknown character '%s'.",
				m_addr_line->text().toStdString(), std::string_view{&wstr[pos], 1});
			return;
		}

		std::string dst;
		dst.resize(wstr.size() / 2);

		for (usz pos = 0; pos < wstr.size() / 2; pos++)
		{
			uchar value = 0;
			std::from_chars(wstr.data() + pos * 2, wstr.data() + (pos + 1) * 2, value, 16);
			std::memcpy(dst.data() + pos, &value, 1);
		}

		wstr = std::move(dst);
		break;
	}
	case as_f64:
	{
		char* end{};
		be_t<f64> value = std::strtod(wstr.data(), &end);

		if (end != wstr.data() + wstr.size())
		{
			gui_log.error("String '%s' cannot be interpreted as double.", wstr);
			return;
		}

		wstr.resize(sizeof(value));
		std::memcpy(wstr.data(), &value, sizeof(value));
		break;
	}
	case as_f32:
	{
		char* end{};
		be_t<f32> value = std::strtof(wstr.data(), &end);

		if (end != wstr.data() + wstr.size())
		{
			gui_log.error("String '%s' cannot be interpreted as float.", wstr);
			return;
		}

		wstr.resize(sizeof(value));
		std::memcpy(wstr.data(), &value, sizeof(value));
		break;
	}
	default: ensure(false);
	}

	// Search the address space for the string
	atomic_t<u32> found = 0;
	atomic_t<u32> avail_addr = 0;

	// There's no need for so many threads
	const u32 max_threads = utils::aligned_div(utils::get_thread_count(), 2);

	static constexpr u32 block_size = 0x2000000;

	vm::reader_lock rlock;

	 const named_thread_group workers("String Searcher "sv, max_threads, [&]()
	{
		u32 local_found = 0;

		u32 addr = 0;
		bool ok = false;

		while (true)
		{
			if (!(addr % block_size))
			{
				std::tie(addr, ok) = avail_addr.fetch_op([](u32& val)
				{
					if (val <= 0 - block_size)
					{
						// Iterate in 32MB blocks
						val += block_size;

						if (!val) val = -1; // Overflow detection

						return true;
					}

					return false;
				});
			}

			if (!ok)
			{
				break;
			}

			if (![&addr = addr]()
			{
				// Skip unmapped memory
				for (const u32 end = utils::align(addr + 1, block_size) - 0x1000; !vm::check_addr(addr, 0); addr += 0x1000)
				{
					if (addr == end)
					{
						return false;
					}
				}

				return true;
			}())
			{
				if (addr == 0u - 0x1000)
				{
					break;
				}

				// The entire block is unmapped
				addr += 0x1000;
				continue;
			}

			u64 addr_max = addr;

			const u64 end_mem = std::min<u64>(utils::align<u64>(addr + 1, block_size) + 0x1000, UINT32_MAX);

			// Determine allocation size quickly
			while (addr_max < end_mem && vm::check_addr(static_cast<u32>(addr_max), vm::page_1m_size))
			{
				addr_max += 0x100000;
			}

			while (addr_max < end_mem && vm::check_addr(static_cast<u32>(addr_max), vm::page_64k_size))
			{
				addr_max += 0x10000;
			}

			while (addr_max < end_mem && vm::check_addr(static_cast<u32>(addr_max), 0))
			{
				addr_max += 0x1000;
			}

			std::string_view section{vm::get_super_ptr<const char>(addr), addr_max - addr};

			usz first_char = 0;

			if (case_insensitive)
			{
				while (first_char = section.find_first_of(insensitive_search, first_char), first_char != umax)
				{
					const u32 start = addr + first_char;

					std::string_view test_sv{vm::get_super_ptr<const char>(start), addr_max - start};

					// Do not use allocating functions such as fmt::to_lower
					if (test_sv.size() >= wstr.size() && std::all_of(wstr.begin(), wstr.end(), [&](const char& c) { return c == ::tolower(test_sv[&c - wstr.data()]); }))
					{
						gui_log.success("Found at 0x%08x", start);
						local_found++;
					}

					// Allow overlapping strings
					first_char++;
				}
			} 
			else
			{
				while (first_char = section.find_first_of(wstr[0], first_char), first_char != umax)
				{
					const u32 start = addr + first_char;

					if (std::string_view{vm::get_super_ptr<const char>(start), addr_max - start}.starts_with(wstr))
					{
						gui_log.success("Found at 0x%08x", start);
						local_found++;
					}

					first_char++;
				}
			}

			addr = static_cast<u32>(std::min<u64>(end_mem - 0x1000, addr_max));

			// Check if at last page
			if (addr_max >= 0u - 0x1000) break;
		}

		found += local_found;
	});

	workers.join();

	gui_log.success("Search completed (found %u matches)", +found);
}
