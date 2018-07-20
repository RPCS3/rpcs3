#include "log_frame.h"
#include "qt_utils.h"

#include "stdafx.h"
#include "rpcs3_version.h"
#include "Utilities/sysinfo.h"

#include <QMenu>
#include <QActionGroup>
#include <QScrollBar>
#include <QTabBar>
#include <QVBoxLayout>

#include <deque>
#include "Utilities/sema.h"

extern fs::file g_tty;
extern atomic_t<s64> g_tty_size;
extern std::array<std::deque<std::string>, 16> g_tty_input;
extern std::mutex g_tty_mutex;

constexpr auto qstr = QString::fromStdString;

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

log_frame::log_frame(std::shared_ptr<gui_settings> guiSettings, QWidget *parent)
	: custom_dock_widget(tr("Log"), parent), xgui_settings(guiSettings)
{
	m_tabWidget = new QTabWidget;
	m_tabWidget->setObjectName("tab_widget_log");
	m_tabWidget->tabBar()->setObjectName("tab_bar_log");

	m_log = new QTextEdit(m_tabWidget);
	m_log->setObjectName("log_frame");
	m_log->setReadOnly(true);
	m_log->setContextMenuPolicy(Qt::CustomContextMenu);
	m_log->installEventFilter(this);

	m_tty = new QTextEdit(m_tabWidget);
	m_tty->setObjectName("tty_frame");
	m_tty->setReadOnly(true);
	m_tty->setContextMenuPolicy(Qt::CustomContextMenu);
	m_tty->installEventFilter(this);

	m_tty_input = new QLineEdit();
	if (m_tty_channel >= 0)
	{
		m_tty_input->setPlaceholderText(tr("Channel %0").arg(m_tty_channel));
	}
	else
	{
		m_tty_input->setPlaceholderText(tr("All User Channels"));
	}

	QVBoxLayout* tty_layout = new QVBoxLayout();
	tty_layout->addWidget(m_tty);
	tty_layout->addWidget(m_tty_input);
	tty_layout->setContentsMargins(0, 0, 0, 0);

	m_tty_container = new QWidget();
	m_tty_container->setLayout(tty_layout);

	m_tabWidget->addTab(m_log, tr("Log"));
	m_tabWidget->addTab(m_tty_container, tr("TTY"));

	setWidget(m_tabWidget);

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

void log_frame::SetTTYLogging(bool val)
{
	m_TTYAct->setChecked(val);
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

	m_tty_channel_acts = new QActionGroup(this);

	// Special Channel: All
	QAction* all_channels_act = new QAction(tr("All user channels"), m_tty_channel_acts);
	all_channels_act->setCheckable(true);
	all_channels_act->setChecked(m_tty_channel == -1);
	connect(all_channels_act, &QAction::triggered, [this]()
	{
		m_tty_channel = -1;
		m_tty_input->setPlaceholderText(tr("All user channels"));
	});

	for (int i = 3; i < 16; i++)
	{
		QAction* act = new QAction(tr("Channel %0").arg(i), m_tty_channel_acts);
		act->setCheckable(true);
		act->setChecked(i == m_tty_channel);
		connect(act, &QAction::triggered, [this, i]()
		{
			m_tty_channel = i;
			m_tty_input->setPlaceholderText(tr("Channel %0").arg(m_tty_channel));
		});
	}

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
		menu->addActions(m_logLevels->actions());
		menu->addSeparator();
		menu->addAction(m_stackAct);
		menu->addSeparator();
		menu->addAction(m_TTYAct);
		menu->exec(mapToGlobal(pos));
	});

	connect(m_tty, &QWidget::customContextMenuRequested, [=](const QPoint& pos)
	{
		QMenu* menu = m_tty->createStandardContextMenu();
		menu->addAction(m_clearTTYAct);
		menu->addSeparator();
		menu->addActions(m_tty_channel_acts->actions());
		menu->exec(mapToGlobal(pos));
	});

	connect(m_tabWidget, &QTabWidget::currentChanged, [this](int/* index*/)
	{
		if (m_find_dialog)
			m_find_dialog->close();
	});

	connect(m_tty_input, &QLineEdit::returnPressed, [this]()
	{
		std::string text = m_tty_input->text().toStdString();

		{
			std::lock_guard<std::mutex> lock(g_tty_mutex);

			if (m_tty_channel == -1)
			{
				for (int i = 3; i < 16; i++)
				{
					g_tty_input[i].push_back(text + "\n");
				}
			}
			else
			{
				g_tty_input[m_tty_channel].push_back(text + "\n");
			}
		}

		// Write to tty
		if (m_tty_channel == -1)
		{
			text = "All channels > " + text + "\n";
		}
		else
		{
			text = fmt::format("%s > %s\n", "Ch.%d", m_tty_channel, text);
		}
		g_tty_size -= (1ll << 48);
		g_tty.write(text.c_str(), text.size());
		g_tty_size += (1ll << 48) + text.size();

		m_tty_input->clear();
	});

	LoadSettings();
}

void log_frame::LoadSettings()
{
	SetLogLevel(xgui_settings->GetLogLevel());
	SetTTYLogging(xgui_settings->GetValue(gui::l_tty).toBool());
	m_stack_log = xgui_settings->GetValue(gui::l_stack).toBool();
	m_stackAct->setChecked(m_stack_log);
}

void log_frame::RepaintTextColors()
{
	// Backup old colors
	QColor old_color_stack{ m_color_stack };
	QList<QColor> old_color{ m_color };

	// Get text color. Do this once to prevent possible slowdown
	m_color.clear();
	m_color.append(gui::utils::get_label_color("log_level_always"));
	m_color.append(gui::utils::get_label_color("log_level_fatal"));
	m_color.append(gui::utils::get_label_color("log_level_error"));
	m_color.append(gui::utils::get_label_color("log_level_todo"));
	m_color.append(gui::utils::get_label_color("log_level_success"));
	m_color.append(gui::utils::get_label_color("log_level_warning"));
	m_color.append(gui::utils::get_label_color("log_level_notice"));
	m_color.append(gui::utils::get_label_color("log_level_trace"));

	m_color_stack = gui::utils::get_label_color("log_stack");

	// Repaint TTY with new colors
	m_tty->setTextColor(gui::utils::get_label_color("tty_text"));

	// Repaint log with new colors
	QTextCursor text_cursor{ m_log->document() };
	text_cursor.movePosition(QTextCursor::Start);

	// Go through each line
	while (!text_cursor.atEnd())
	{
		// Jump to the next line, unless this is the first one
		if (!text_cursor.atStart())
			text_cursor.movePosition(QTextCursor::NextBlock);

		// Go through each word in the current line
		while (!text_cursor.atBlockEnd())
		{
			// Remove old selection and select a new character (NextWord has proven to be glitchy here)
			text_cursor.movePosition(QTextCursor::NoMove);
			text_cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);

			// Skip if no color needed
			if (text_cursor.selectedText() == " ")
				continue;

			// Get current chars color
			QTextCharFormat text_format = text_cursor.charFormat();
			QColor col = text_format.foreground().color();

			// Get the new color for this word
			if (col == old_color_stack)
			{
				text_format.setForeground(m_color_stack);
			}
			else
			{
				for (int i = 0; i < old_color.count(); i++)
				{
					if (col == old_color[i])
					{
						text_format.setForeground(m_color[i]);
						break;
					}
				}
			}

			// Reinsert the same text with the new color
			text_cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
			text_cursor.insertText(text_cursor.selectedText(), text_format);
		}
	}
}

void log_frame::UpdateUI()
{
	const auto start = steady_clock::now();

	// Check TTY logs
	while (const u64 size = std::max<s64>(0, g_tty_size.load() - m_tty_file.pos()))
	{
		std::string buf;
		buf.resize(size);
		buf.resize(m_tty_file.read(&buf.front(), buf.size()));

		if (buf.find_first_of('\0') != -1)
		{
			m_tty_file.seek(s64{0} - buf.size(), fs::seek_mode::seek_cur);
			break;
		}

		if (buf.size() && m_TTYAct->isChecked())
		{
			QTextCursor text_cursor{m_tty->document()};
			text_cursor.movePosition(QTextCursor::End);
			text_cursor.insertText(qstr(buf));
		}

		// Limit processing time
		if (steady_clock::now() >= start + 4ms || buf.empty()) break;
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

			// remove the new line because Qt's append adds a new line already.
			text.chop(1);

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
			m_log->setTextColor(m_color[static_cast<int>(packet->sev)]);
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

bool log_frame::eventFilter(QObject* object, QEvent* event)
{
	if (object != m_log && object != m_tty)
	{
		return QDockWidget::eventFilter(object, event);
	}

	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent* e = (QKeyEvent*)event;
		if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_F)
		{
			if (m_find_dialog && m_find_dialog->isVisible())
				m_find_dialog->close();

			m_find_dialog = std::make_unique<find_dialog>((QTextEdit*)object, this);
		}
	}

	return QDockWidget::eventFilter(object, event);
}
