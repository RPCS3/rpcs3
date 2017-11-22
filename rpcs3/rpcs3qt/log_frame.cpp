#include "log_frame.h"

#include "stdafx.h"
#include "rpcs3_version.h"
#include "Utilities/sysinfo.h"

#include <QMenu>
#include <QActionGroup>
#include <QScrollBar>
#include <QVBoxLayout>

constexpr auto qstr = QString::fromStdString;
inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }

struct gui_listener : logs::listener
{
	atomic_t<logs::level> enabled{logs::level::_uninit};

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
		, last(new packet)
		, read(+last)
	{
		// Self-registration
		logs::listener::add(this);
	}

	~gui_listener()
	{
		delete read;
	}

	void log(u64 stamp, const logs::message& msg, const std::string& prefix, const std::string& text)
	{
		Q_UNUSED(stamp);

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

			if (msg.ch && '\0' != *msg.ch->name)
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

log_frame::log_frame(std::shared_ptr<gui_settings> guiSettings, QWidget *parent) : QDockWidget(tr("Log"), parent), xgui_settings(guiSettings)
{
	QTabWidget* tabWidget = new QTabWidget;

	m_log = new QTextEdit(tabWidget);
	m_log->setObjectName("log_frame");
	m_log->setReadOnly(true);
	m_log->setContextMenuPolicy(Qt::CustomContextMenu);

	m_tty = new QTextEdit(tabWidget);
	m_tty->setObjectName("tty_frame");
	m_tty->setReadOnly(true);
	m_tty->setContextMenuPolicy(Qt::CustomContextMenu);

	m_tty_input = new QLineEdit(tabWidget);
	m_tty_input->setObjectName("tty_input");

	QVBoxLayout* tty_layout = new QVBoxLayout;
	tty_layout->setContentsMargins(0,0,0,0);
	tty_layout->addWidget(m_tty);
	tty_layout->addWidget(m_tty_input);

	QWidget* tty = new QWidget(tabWidget);
	tty->setLayout(tty_layout);

	tabWidget->addTab(m_log, tr("Log"));
	tabWidget->addTab(tty, tr("TTY"));

	setWidget(tabWidget);

	// Open or create TTY.log
	m_tty_file.open(fs::get_config_dir() + "TTY.log", fs::read + fs::create);

	CreateAndConnectActions();

	// Check for updates every ~10 ms
	QTimer *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &log_frame::UpdateUI);
	timer->start(10);
}

void log_frame::SetLogLevel(logs::level lev)
{
	switch (lev)
	{
	case logs::level::always:
	case logs::level::fatal:
	{
		m_fatalAct->trigger();
		break;
	}
	case logs::level::error:
	{
		m_errorAct->trigger();
		break;
	}
	case logs::level::todo:
	{
		m_todoAct->trigger();
		break;
	}
	case logs::level::success:
	{
		m_successAct->trigger();
		break;
	}
	case logs::level::warning:
	{
		m_warningAct->trigger();
		break;
	}
	case logs::level::notice:
	{
		m_noticeAct->trigger();
		break;
	}
	case logs::level::trace:
	{
		m_traceAct->trigger();
		break;
	}
	default:
		m_warningAct->trigger();
	}
}

void log_frame::CreateAndConnectActions()
{
	// I, for one, welcome our lambda overlord
	// It's either this or a signal mapper
	// Then, probably making a list of these actions so that it's easier to iterate to generate the mapper.
	auto l_initAct = [this](QAction* act, logs::level logLevel)
	{
		act->setCheckable(true);
		
		// This sets the log level properly when the action is triggered.
		connect(act, &QAction::triggered, [this, logLevel]()
		{
			s_gui_listener.enabled = std::max(logLevel, logs::level::fatal);
			xgui_settings->SetValue(gui::l_level, static_cast<uint>(logLevel));
		});
	};

	m_clearAct = new QAction(tr("Clear"), this);
	connect(m_clearAct, &QAction::triggered, m_log, &QTextEdit::clear);

	m_clearTTYAct = new QAction(tr("Clear"), this);
	connect(m_clearTTYAct, &QAction::triggered, m_tty, &QTextEdit::clear);

	// Action groups make these actions mutually exclusive.
	m_logLevels = new QActionGroup(this);
	m_nothingAct = new QAction(tr("Nothing"), m_logLevels);
	m_nothingAct->setVisible(false);
	m_fatalAct = new QAction(tr("Fatal"), m_logLevels);
	m_errorAct = new QAction(tr("Error"), m_logLevels);
	m_todoAct = new QAction(tr("Todo"), m_logLevels);
	m_successAct = new QAction(tr("Success"), m_logLevels);
	m_warningAct = new QAction(tr("Warning"), m_logLevels);
	m_noticeAct = new QAction(tr("Notice"), m_logLevels);
	m_traceAct = new QAction(tr("Trace"), m_logLevels);

	m_stackAct = new QAction(tr("Stack Mode"), this);
	m_stackAct->setCheckable(true);
	connect(m_stackAct, &QAction::toggled, xgui_settings.get(), [=](bool checked)
	{
		xgui_settings->SetValue(gui::l_stack, checked);
		m_stack_log = checked;
	});

	m_TTYAct = new QAction(tr("TTY"), this);
	m_TTYAct->setCheckable(true);
	connect(m_TTYAct, &QAction::triggered, xgui_settings.get(), [=](bool checked)
	{
		xgui_settings->SetValue(gui::l_tty, checked);
		m_log_tty = checked;
	});

	m_TTYLogAct = new QAction(tr("Show TTY in Log"), this);
	m_TTYLogAct->setCheckable(true);
	connect(m_TTYLogAct, &QAction::triggered, xgui_settings.get(), [=](bool checked)
	{
		xgui_settings->SetValue(gui::l_tty_log, checked);
		m_show_tty_in_log = checked;
	});

	l_initAct(m_nothingAct, logs::level::fatal);
	l_initAct(m_fatalAct, logs::level::fatal);
	l_initAct(m_errorAct, logs::level::error);
	l_initAct(m_todoAct, logs::level::todo);
	l_initAct(m_successAct, logs::level::success);
	l_initAct(m_warningAct, logs::level::warning);
	l_initAct(m_noticeAct, logs::level::notice);
	l_initAct(m_traceAct, logs::level::trace);

	connect(m_log, &QWidget::customContextMenuRequested, [=](const QPoint& pos)
	{
		QMenu* menu = m_log->createStandardContextMenu();
		menu->addAction(m_clearAct);
		menu->addSeparator();
		menu->addActions({ m_nothingAct, m_fatalAct, m_errorAct, m_todoAct, m_successAct, m_warningAct, m_noticeAct, m_traceAct });
		menu->addSeparator();
		menu->addAction(m_stackAct);
		menu->addSeparator();
		menu->addAction(m_TTYAct);
		menu->addAction(m_TTYLogAct);
		menu->exec(mapToGlobal(pos));
	});

	connect(m_tty, &QWidget::customContextMenuRequested, [=](const QPoint& pos)
	{
		QMenu* menu = m_tty->createStandardContextMenu();
		menu->addAction(m_clearTTYAct);
		menu->exec(mapToGlobal(pos));
	});

	connect(m_tty_input, &QLineEdit::editingFinished, [&]()
	{
		if (m_tty_input->text().isEmpty()) return;
		std::string text = sstr(m_tty_input->text());
		m_tty_input->clear();
		LOG_NOTICE(GENERAL, "TTY Input entered: %s", text);
	});

	LoadSettings();
}

void log_frame::LoadSettings()
{
	SetLogLevel(xgui_settings->GetLogLevel());

	m_log_tty = xgui_settings->GetValue(gui::l_tty).toBool();
	m_TTYAct->setChecked(m_log_tty);

	m_show_tty_in_log = xgui_settings->GetValue(gui::l_tty_log).toBool();
	m_TTYLogAct->setChecked(m_show_tty_in_log);

	m_stack_log = xgui_settings->GetValue(gui::l_stack).toBool();
	m_stackAct->setChecked(m_stack_log);
}

//TODO: repaint old text with new colors
void log_frame::RepaintTextColors()
{
	// Get text color. Do this once to prevent possible slowdown
	m_color.clear();
	m_color.append(gui::get_Label_Color("log_level_always"));
	m_color.append(gui::get_Label_Color("log_level_fatal"));
	m_color.append(gui::get_Label_Color("log_level_error"));
	m_color.append(gui::get_Label_Color("log_level_todo"));
	m_color.append(gui::get_Label_Color("log_level_success"));
	m_color.append(gui::get_Label_Color("log_level_warning"));
	m_color.append(gui::get_Label_Color("log_level_notice"));
	m_color.append(gui::get_Label_Color("log_level_trace"));

	m_color_stack = gui::get_Label_Color("log_stack");

	m_tty->setTextColor(gui::get_Label_Color("tty_text"));
}

void log_frame::UpdateUI()
{
	std::vector<char> buf(4096);

	// Get UTF-8 string from file
	auto get_utf8 = [&](const fs::file& file, u64 size) -> QString
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
			{ // Copying is expensive-- O(i)-- but I suspect this corruption will be exceptionally unlikely.
				file.seek(i - size, fs::seek_cur);
				std::vector<char> sub(&buf[0], &buf[i]);
				return QString(sub.data());
			}
		}
		return QString(buf.data());
	};

	auto write_to_log = [&](const QString& text, const QColor& color)
	{
		// save old log state
		QScrollBar *sb = m_log->verticalScrollBar();
		bool isMax = sb->value() == sb->maximum();
		int sb_pos = sb->value();
		int sel_pos = m_log->textCursor().position();
		int sel_start = m_log->textCursor().selectionStart();
		int sel_end = m_log->textCursor().selectionEnd();

		// clear selection or else it will get colorized as well
		QTextCursor c = m_log->textCursor();
		c.clearSelection();
		m_log->setTextCursor(c);

		QString suffix;
		bool isSame = text == m_old_text;

		// create counter suffix and remove recurring line if needed
		if (m_stack_log)
		{
			if (isSame)
			{
				m_log_counter++;
				suffix = QString(" x%1").arg(m_log_counter);
				m_log->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
				m_log->moveCursor(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
				m_log->moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
				m_log->textCursor().removeSelectedText();
				m_log->textCursor().deletePreviousChar();
			}
			else
			{
				m_log_counter = 1;
				m_old_text = text;
			}
		}

		// add actual log message
		m_log->setTextColor(color);
		m_log->append(text);

		// add counter suffix if needed
		if (m_stack_log && isSame)
		{
			m_log->setTextColor(m_color_stack);
			m_log->insertPlainText(suffix);
		}

		// if we mark text from right to left we need to swap sides (start is always smaller than end)
		if (sel_pos < sel_end)
		{
			std::swap(sel_start, sel_end);
		}

		// reset old text cursor and selection
		c.setPosition(sel_start);
		c.setPosition(sel_end, QTextCursor::KeepAnchor);
		m_log->setTextCursor(c);

		// set scrollbar to max means auto-scroll
		sb->setValue(isMax ? sb->maximum() : sb_pos);
	};

	const auto start = steady_clock::now();

	// Check TTY logs

	int tty_count = -1;
	static QColor tty_col = m_color[static_cast<int>(logs::level::notice)];

	while (const u64 size = std::min<u64>(buf.size(), m_tty_file.size() - m_tty_file.pos()))
	{
		QString text = get_utf8(m_tty_file, size);

		if (m_log_tty)
		{
			if (m_show_tty_in_log && ++tty_count == 0)
			{
				m_log->setTextColor(tty_col);
				m_log->append("--- TTY begin ---");
			}

			// remove the new line because Qt's append adds a new line already.
			text.chop(1);
			m_tty->append(text);

			if (m_show_tty_in_log && logs::level::notice <= s_gui_listener.enabled)
			{
				write_to_log(text, tty_col);
			}
		}
		// Limit processing time
		if (steady_clock::now() >= start + 4ms || text.isEmpty()) break;
	}

	if (m_show_tty_in_log && tty_count >= 0)
	{
		m_log->setTextColor(tty_col);
		m_log->append("--- TTY end ---");
	}

	// Check main logs
	while (const auto packet = s_gui_listener.read->next.load())
	{
		// Confirm log level
		if (packet->sev <= s_gui_listener.enabled)
		{
			QString text;
			switch (packet->sev)
			{
			case logs::level::always: break;
			case logs::level::fatal: text = "F "; break;
			case logs::level::error: text = "E "; break;
			case logs::level::todo: text = "U "; break;
			case logs::level::success: text = "S "; break;
			case logs::level::warning: text = "W "; break;
			case logs::level::notice: text = "! "; break;
			case logs::level::trace: text = "T "; break;
			default: continue;
			}

			// Print UTF-8 text.
			text += qstr(packet->msg);

			// remove the new line because Qt's append adds a new line already.
			text.chop(1);

			write_to_log(text, m_color[static_cast<int>(packet->sev)]);
		}

		// Drop packet
		s_gui_listener.pop();

		// Limit processing time
		if (steady_clock::now() >= start + 7ms) break;
	}
}

void log_frame::closeEvent(QCloseEvent *event)
{
	QDockWidget::closeEvent(event);
	Q_EMIT LogFrameClosed();
}
