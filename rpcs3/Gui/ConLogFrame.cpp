#include "stdafx.h"
#include "stdafx_gui.h"
#include "Gui/ConLogFrame.h"

#include <chrono>

enum
{
	id_log_copy,  // Copy log to ClipBoard
	id_log_clear, // Clear log
	id_log_level,
	id_log_level7 = id_log_level + 7,
	id_log_tty,
	id_timer,
};

BEGIN_EVENT_TABLE(LogFrame, wxPanel)
EVT_CLOSE(LogFrame::OnQuit)
EVT_TIMER(id_timer, LogFrame::OnTimer)
END_EVENT_TABLE()

LogFrame::LogFrame(wxWindow* parent)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(600, 500))
	, m_tabs(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS)
	, m_log(new wxTextCtrl(&m_tabs, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2))
	, m_tty(new wxTextCtrl(&m_tabs, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2))
	, m_timer(this, id_timer)
	, m_cfg_level(g_gui_cfg["Log Level"])
	, m_cfg_tty(g_gui_cfg["Log TTY"])
{
	// Open or create RPCS3.log; TTY.log
	m_log_file.open(fs::get_config_dir() + "RPCS3.log", fs::read + fs::create);
	m_tty_file.open(fs::get_config_dir() + "TTY.log",   fs::read + fs::create);

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

	Show();

	// Check for updates every ~10 ms
	m_timer.Start(10);
}

LogFrame::~LogFrame()
{
}

bool LogFrame::Close(bool force)
{
	return wxWindowBase::Close(force);
}

void LogFrame::OnQuit(wxCloseEvent& event)
{
	event.Skip();
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
		m_log_file.seek(0, fs::seek_end);
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
			break;
		}
	}
	}

	save_gui_cfg();
	event.Skip();
}

void LogFrame::OnTimer(wxTimerEvent& event)
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

	const auto start = std::chrono::high_resolution_clock::now();

	// Check TTY logs
	while (const u64 size = std::min<u64>(sizeof(buf), m_tty_file.size() - m_tty_file.pos()))
	{
		const wxString& text = get_utf8(m_tty_file, size);

		if (get_cfg_tty()) m_tty->AppendText(text);

		// Limit processing time
		if (std::chrono::high_resolution_clock::now() >= start + 4ms || text.empty()) break;
	}

	// Check main logs
	while (const u64 size = std::min<u64>(sizeof(buf), m_log_file.size() - m_log_file.pos()))
	{
		const wxString& text = get_utf8(m_log_file, size);

		// Append text if necessary
		auto flush_logs = [&](u64 start, u64 pos)
		{
			if (pos != start && m_level <= get_cfg_level()) // TODO
			{
				m_log->SetDefaultStyle(m_color);
				m_log->AppendText(text.substr(start, pos - start));
			}
		};

		// Parse log level formatting
		for (std::size_t start = 0, pos = 0;; pos++)
		{
			if (pos < text.size() && text[pos] == L'·')
			{
				if (text.size() - pos <= 3)
				{
					// Cannot get log formatting: abort
					m_log_file.seek(0 - text.substr(pos).ToUTF8().length(), fs::seek_cur);

					flush_logs(start, pos);
					break;
				}

				if (text[pos + 2] == ' ')
				{
					logs::level level;
					wxColour color;

					switch (text[pos + 1].GetValue())
					{
					case 'A': level = logs::level::always; color.Set(0x00, 0xFF, 0xFF); break; // Cyan
					case 'F': level = logs::level::fatal; color.Set(0xFF, 0x00, 0xFF); break; // Fuchsia
					case 'E': level = logs::level::error; color.Set(0xFF, 0x00, 0x00); break; // Red
					case 'U': level = logs::level::todo; color.Set(0xFF, 0x60, 0x00); break; // Orange
					case 'S': level = logs::level::success; color.Set(0x00, 0xFF, 0x00); break; // Green
					case 'W': level = logs::level::warning; color.Set(0xFF, 0xFF, 0x00); break; // Yellow
					case '!': level = logs::level::notice; color.Set(0xFF, 0xFF, 0xFF); break; // White
					case 'T': level = logs::level::trace; color.Set(0x80, 0x80, 0x80); break; // Gray
					default: continue;
					}

					flush_logs(start, pos);

					start = pos + 3;
					m_level = level;
					m_color = color;
				}
			}

			if (pos >= text.size())
			{
				flush_logs(start, pos);
				break;
			}
		}

		// Limit processing time
		if (std::chrono::high_resolution_clock::now() >= start + 7ms || text.empty()) break;
	}
}
