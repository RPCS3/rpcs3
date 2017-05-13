#include "stdafx.h"
#include "stdafx_gui.h"
#include "Gui/ConLogFrame.h"

#include "rpcs3_version.h"
#include <chrono>

struct gui_listener : logs::listener
{
	atomic_t<logs::level> enabled{};

	struct packet
	{
		atomic_t<packet*> next{};

		logs::level sev{};
		std::string msg;

		~packet()
		{
			for (auto ptr = next.raw(); UNLIKELY(ptr);)
			{
				delete std::exchange(ptr, std::exchange(ptr->next.raw(), nullptr));
			}
		}
	};

	atomic_t<packet*> last; // Packet for writing another packet
	atomic_t<packet*> read; // Packet for reading

	gui_listener()
		: logs::listener()
	{
		// Initialize packets
		read = new packet;
		last = new packet;
		read->next = last.load();
		last->msg = fmt::format("RPCS3 v%s\n", rpcs3::version.to_string());

		// Self-registration
		logs::listener::add(this);
	}

	~gui_listener()
	{
		delete read;
	}

	void log(u64 stamp, const logs::message& msg, const std::string& prefix, const std::string& text)
	{
		if (msg.sev <= enabled)
		{
			const auto _new = new packet;
			_new->sev = msg.sev;

			if (prefix.size() > 0)
			{
				_new->msg += "{";
				_new->msg += prefix;
				_new->msg += "} ";
			}

			if ('\0' != *msg.ch->name)
			{
				_new->msg += msg.ch->name;
				_new->msg += msg.sev == logs::level::todo ? " TODO: " : ": ";
			}
			else if (msg.sev == logs::level::todo)
			{
				_new->msg += "TODO: ";
			}

			_new->msg += text;
			_new->msg += '\n';

			last.exchange(_new)->next = _new;
		}
	}

	void pop()
	{
		if (const auto head = read->next.exchange(nullptr))
		{
			delete read.exchange(head);
		}
	}

	void clear()
	{
		while (read->next)
		{
			pop();
		}
	}
};

// GUI Listener instance
static gui_listener s_gui_listener;

enum
{
	id_log_copy,  // Copy log to ClipBoard
	id_log_clear, // Clear log
	id_log_level,
	id_log_level7 = id_log_level + 7,
	id_log_tty,
	id_timer,
};

LogFrame::LogFrame(wxWindow* parent)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(600, 500))
	, m_tabs(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS)
	, m_log(new wxTextCtrl(&m_tabs, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2))
	, m_tty(new wxTextCtrl(&m_tabs, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2))
	, m_cfg_level(g_gui_cfg["Log Level"])
	, m_cfg_tty(g_gui_cfg["Log TTY"])
{
	// Open or create TTY.log
	m_tty_file.open(fs::get_config_dir() + "TTY.log", fs::read + fs::create);

	m_tty->SetBackgroundColour(wxColour("Black"));
	m_log->SetBackgroundColour(wxColour("Black"));
	m_tty->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_tty->SetDefaultStyle(wxColour(255, 255, 255));
	m_tabs.AddPage(m_log, "Log");
	m_tabs.AddPage(m_tty, "TTY");

	wxBoxSizer* s_main = new wxBoxSizer(wxVERTICAL);
	s_main->Add(&m_tabs, 1, wxEXPAND);
	SetSizer(s_main);
	Layout();

	m_log->Bind(wxEVT_RIGHT_DOWN, &LogFrame::OnRightClick, this);
	Bind(wxEVT_MENU, &LogFrame::OnContextMenu, this, id_log_clear);
	Bind(wxEVT_MENU, &LogFrame::OnContextMenu, this, id_log_copy);
	Bind(wxEVT_MENU, &LogFrame::OnContextMenu, this, id_log_level, id_log_level + 7);
	Bind(wxEVT_MENU, &LogFrame::OnContextMenu, this, id_log_tty);

	Bind(wxEVT_CLOSE_WINDOW, [](wxCloseEvent& event) { event.Skip(); });

	Show();

	// Update listener info
	s_gui_listener.enabled = get_cfg_level();
}

LogFrame::~LogFrame()
{
}

bool LogFrame::Close(bool force)
{
	return wxWindowBase::Close(force);
}

// Deals with the RightClick on Log Console, shows up the Context Menu.
void LogFrame::OnRightClick(wxMouseEvent& event)
{
	wxMenu* menu = new wxMenu();

	menu->Append(id_log_copy, "&Copy");
	menu->Append(id_log_clear, "C&lear");
	menu->AppendSeparator();
	menu->AppendRadioItem(id_log_level + 0, "Nothing");
	menu->AppendRadioItem(id_log_level + 1, "Fatal");
	menu->AppendRadioItem(id_log_level + 2, "Error");
	menu->AppendRadioItem(id_log_level + 3, "Todo");
	menu->AppendRadioItem(id_log_level + 4, "Success");
	menu->AppendRadioItem(id_log_level + 5, "Warning");
	menu->AppendRadioItem(id_log_level + 6, "Notice");
	menu->AppendRadioItem(id_log_level + 7, "Trace");
	menu->AppendSeparator();
	menu->AppendCheckItem(id_log_tty, "TTY");

	menu->Check(id_log_level + static_cast<uint>(get_cfg_level()), true);
	menu->Check(id_log_tty, get_cfg_tty());

	PopupMenu(menu);
}

// Well you can bind more than one control to a single handler.
void LogFrame::OnContextMenu(wxCommandEvent& event)
{
	switch (auto id = event.GetId())
	{
	case id_log_clear:
	{
		m_log->Clear();
		s_gui_listener.clear();
		break;
	}

	case id_log_copy:
	{
		if (wxTheClipboard->Open())
		{
			m_tdo = new wxTextDataObject(m_log->GetStringSelection());
			if (m_tdo->GetTextLength() > 0)
			{
				wxTheClipboard->SetData(new wxTextDataObject(m_log->GetStringSelection()));
			}
			wxTheClipboard->Close();
		}
		break;
	}

	case id_log_tty:
	{
		m_cfg_tty = !get_cfg_tty();
		break;
	}

	default:
	{
		if (id >= id_log_level && id < id_log_level + 8)
		{
			m_cfg_level = id - id_log_level;
			s_gui_listener.enabled = static_cast<logs::level>(id - id_log_level);
			break;
		}
	}
	}

	save_gui_cfg();
	event.Skip();
}

void LogFrame::UpdateUI()
{
	std::vector<char> buf(4096);

	// Get UTF-8 string from file
	auto get_utf8 = [&](const fs::file& file, u64 size) -> wxString
	{
		size = file.read(buf.data(), size);

		for (u64 i = 0; i < size; i++)
		{
			// Get UTF-8 sequence length (no real validation performed)
			const u64 tail =
				(buf[i] & 0xF0) == 0xF0 ? 3 :
				(buf[i] & 0xE0) == 0xE0 ? 2 :
				(buf[i] & 0xC0) == 0xC0 ? 1 : 0;

			if (i + tail >= size)
			{
				file.seek(i - size, fs::seek_cur);
				return wxString::FromUTF8(buf.data(), i);
			}
		}

		return wxString::FromUTF8(buf.data(), size);
	};

	const auto start = steady_clock::now();

	// Check TTY logs
	while (const u64 size = std::min<u64>(buf.size(), m_tty_file.size() - m_tty_file.pos()))
	{
		const wxString& text = get_utf8(m_tty_file, size);

		if (get_cfg_tty()) m_tty->AppendText(text);

		// Limit processing time
		if (steady_clock::now() >= start + 4ms || text.empty()) break;
	}

	// Check main logs
	while (const auto packet = s_gui_listener.read->next.load())
	{
		// Confirm log level
		if (packet->sev <= s_gui_listener.enabled)
		{
			// Get text color
			wxColour color;
			wxString text;
			switch (packet->sev)
			{
			case logs::level::always: color.Set(0x00, 0xFF, 0xFF); break; // Cyan
			case logs::level::fatal: text = "F "; color.Set(0xFF, 0x00, 0xFF); break; // Fuchsia
			case logs::level::error: text = "E "; color.Set(0xFF, 0x00, 0x00); break; // Red
			case logs::level::todo: text = "U "; color.Set(0xFF, 0x60, 0x00); break; // Orange
			case logs::level::success: text = "S "; color.Set(0x00, 0xFF, 0x00); break; // Green
			case logs::level::warning: text = "W "; color.Set(0xFF, 0xFF, 0x00); break; // Yellow
			case logs::level::notice: text = "! "; color.Set(0xFF, 0xFF, 0xFF); break; // White
			case logs::level::trace: text = "T "; color.Set(0x80, 0x80, 0x80); break; // Gray
			default: continue;
			}

			// Print UTF-8 text
			text += wxString::FromUTF8(packet->msg.data(), packet->msg.size());
			m_log->SetDefaultStyle(color);
			m_log->AppendText(text);
		}

		// Drop packet
		s_gui_listener.pop();

		// Limit processing time
		if (steady_clock::now() >= start + 7ms) break;
	}
}
