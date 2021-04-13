#include "log_frame.h"
#include "qt_utils.h"
#include "gui_settings.h"

#include "rpcs3_version.h"
#include "Utilities/mutex.h"
#include "Utilities/lockless.h"

#include <QMenu>
#include <QActionGroup>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QTimer>
#include <QStringBuilder>

#include <sstream>
#include <deque>
#include <mutex>

extern fs::file g_tty;
extern atomic_t<s64> g_tty_size;
extern std::array<std::deque<std::string>, 16> g_tty_input;
extern std::mutex g_tty_mutex;

constexpr auto qstr = QString::fromStdString;

struct gui_listener : logs::listener
{
	atomic_t<logs::level> enabled{logs::level{UINT_MAX}};

	struct packet_t
	{
		logs::level sev{};
		std::string msg;
	};

	lf_queue_slice<packet_t> pending;

	lf_queue<packet_t> queue;

	atomic_t<bool> show_prefix{false};

	gui_listener()
		: logs::listener()
	{
		// Self-registration
		logs::listener::add(this);
	}

	~gui_listener()
	{
	}

	void log(u64 stamp, const logs::message& msg, const std::string& prefix, const std::string& text) override
	{
		Q_UNUSED(stamp)

		if (msg.sev <= enabled)
		{
			packet_t p,* _new = &p;
			_new->sev = msg.sev;

			if (show_prefix && !prefix.empty())
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

			queue.push(std::move(p));
		}
	}

	void pop()
	{
		pending.pop_front();
	}

	packet_t* get()
	{
		if (packet_t* _head = pending.get())
		{
			return _head;
		}

		pending = queue.pop_all();
		return pending.get();
	}
};

// GUI Listener instance
static gui_listener s_gui_listener;

log_frame::log_frame(std::shared_ptr<gui_settings> guiSettings, QWidget *parent)
	: custom_dock_widget(tr("Log"), parent), m_gui_settings(std::move(guiSettings))
{
	const int max_block_count_log = m_gui_settings->GetValue(gui::l_limit).toInt();
	const int max_block_count_tty = m_gui_settings->GetValue(gui::l_limit_tty).toInt();

	m_tabWidget = new QTabWidget;
	m_tabWidget->setObjectName(QStringLiteral("tab_widget_log"));
	m_tabWidget->tabBar()->setObjectName(QStringLiteral("tab_bar_log"));

	m_log = new QPlainTextEdit(m_tabWidget);
	m_log->setObjectName(QStringLiteral("log_frame"));
	m_log->setReadOnly(true);
	m_log->setContextMenuPolicy(Qt::CustomContextMenu);
	m_log->document()->setMaximumBlockCount(max_block_count_log);
	m_log->installEventFilter(this);

	m_tty = new QPlainTextEdit(m_tabWidget);
	m_tty->setObjectName(QStringLiteral("tty_frame"));
	m_tty->setReadOnly(true);
	m_tty->setContextMenuPolicy(Qt::CustomContextMenu);
	m_tty->document()->setMaximumBlockCount(max_block_count_tty);
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
	m_tty_file.open(fs::get_cache_dir() + "TTY.log", fs::read + fs::create);

	CreateAndConnectActions();

	// Check for updates every ~10 ms
	QTimer *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &log_frame::UpdateUI);
	timer->start(10);
}

void log_frame::SetLogLevel(logs::level lev) const
{
	switch (lev)
	{
	case logs::level::always:
	case logs::level::fatal:
	{
		m_fatal_act->trigger();
		break;
	}
	case logs::level::error:
	{
		m_error_act->trigger();
		break;
	}
	case logs::level::todo:
	{
		m_todo_act->trigger();
		break;
	}
	case logs::level::success:
	{
		m_success_act->trigger();
		break;
	}
	case logs::level::warning:
	{
		m_warning_act->trigger();
		break;
	}
	case logs::level::notice:
	{
		m_notice_act->trigger();
		break;
	}
	case logs::level::trace:
	{
		m_trace_act->trigger();
		break;
	}
	}
}

void log_frame::SetTTYLogging(bool val) const
{
	m_tty_act->setChecked(val);
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
			m_gui_settings->SetValue(gui::l_level, static_cast<uint>(logLevel));
		});
	};

	m_clear_act = new QAction(tr("Clear"), this);
	connect(m_clear_act, &QAction::triggered, [this]()
	{
		m_old_log_text = "";
		m_log->clear();
	});

	m_clear_tty_act = new QAction(tr("Clear"), this);
	connect(m_clear_tty_act, &QAction::triggered, [this]()
	{
		m_old_tty_text = "";
		m_tty->clear();
	});

	m_stack_act_tty = new QAction(tr("Stack Mode (TTY)"), this);
	m_stack_act_tty->setCheckable(true);
	connect(m_stack_act_tty, &QAction::toggled, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::l_stack_tty, checked);
		m_stack_tty = checked;
	});

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
	m_log_level_acts = new QActionGroup(this);
	m_nothing_act = new QAction(tr("Nothing"), m_log_level_acts);
	m_nothing_act->setVisible(false);
	m_fatal_act = new QAction(tr("Fatal"), m_log_level_acts);
	m_error_act = new QAction(tr("Error"), m_log_level_acts);
	m_todo_act = new QAction(tr("Todo"), m_log_level_acts);
	m_success_act = new QAction(tr("Success"), m_log_level_acts);
	m_warning_act = new QAction(tr("Warning"), m_log_level_acts);
	m_notice_act = new QAction(tr("Notice"), m_log_level_acts);
	m_trace_act = new QAction(tr("Trace"), m_log_level_acts);

	m_stack_act_log = new QAction(tr("Stack Mode (Log)"), this);
	m_stack_act_log->setCheckable(true);
	connect(m_stack_act_log, &QAction::toggled, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::l_stack, checked);
		m_stack_log = checked;
	});

	m_show_prefix_act = new QAction(tr("Show Thread Prefix"), this);
	m_show_prefix_act->setCheckable(true);
	connect(m_show_prefix_act, &QAction::toggled, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::l_prefix, checked);
		s_gui_listener.show_prefix = checked;
	});

	m_tty_act = new QAction(tr("Enable TTY"), this);
	m_tty_act->setCheckable(true);
	connect(m_tty_act, &QAction::triggered, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::l_tty, checked);
	});

	l_initAct(m_nothing_act, logs::level::fatal);
	l_initAct(m_fatal_act, logs::level::fatal);
	l_initAct(m_error_act, logs::level::error);
	l_initAct(m_todo_act, logs::level::todo);
	l_initAct(m_success_act, logs::level::success);
	l_initAct(m_warning_act, logs::level::warning);
	l_initAct(m_notice_act, logs::level::notice);
	l_initAct(m_trace_act, logs::level::trace);

	connect(m_log, &QWidget::customContextMenuRequested, [this](const QPoint& pos)
	{
		QMenu* menu = m_log->createStandardContextMenu();
		menu->addAction(m_clear_act);
		menu->addSeparator();
		menu->addActions(m_log_level_acts->actions());
		menu->addSeparator();
		menu->addAction(m_stack_act_log);
		menu->addAction(m_show_prefix_act);
		menu->exec(m_log->viewport()->mapToGlobal(pos));
	});

	connect(m_tty, &QWidget::customContextMenuRequested, [this](const QPoint& pos)
	{
		QMenu* menu = m_tty->createStandardContextMenu();
		menu->addAction(m_clear_tty_act);
		menu->addSeparator();
		menu->addAction(m_tty_act);
		menu->addAction(m_stack_act_tty);
		menu->addSeparator();
		menu->addActions(m_tty_channel_acts->actions());
		menu->exec(m_tty->viewport()->mapToGlobal(pos));
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
			std::lock_guard lock(g_tty_mutex);

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
			text = fmt::format("Ch.%d > %s\n", m_tty_channel, text);
		}

		if (g_tty)
		{
			g_tty_size -= (1ll << 48);
			g_tty.write(text.c_str(), text.size());
			g_tty_size += (1ll << 48) + text.size();
		}

		m_tty_input->clear();
	});

	LoadSettings();
}

void log_frame::LoadSettings()
{
	SetLogLevel(m_gui_settings->GetLogLevel());
	SetTTYLogging(m_gui_settings->GetValue(gui::l_tty).toBool());
	m_stack_log = m_gui_settings->GetValue(gui::l_stack).toBool();
	m_stack_tty = m_gui_settings->GetValue(gui::l_stack_tty).toBool();
	m_stack_act_log->setChecked(m_stack_log);
	m_stack_act_tty->setChecked(m_stack_tty);

	s_gui_listener.show_prefix = m_gui_settings->GetValue(gui::l_prefix).toBool();
	m_show_prefix_act->setChecked(s_gui_listener.show_prefix);

	if (m_log)
	{
		m_log->document()->setMaximumBlockCount(m_gui_settings->GetValue(gui::l_limit).toInt());
	}

	if (m_tty)
	{
		m_tty->document()->setMaximumBlockCount(m_gui_settings->GetValue(gui::l_limit_tty).toInt());
	}
}

void log_frame::RepaintTextColors()
{
	// Backup old colors
	QList<QColor> old_colors = m_color;
	QColor old_stack_color = m_color_stack;

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

	// Use new colors if the old colors weren't set yet
	if (old_colors.empty())
	{
		old_colors = m_color;
	}

	if (!old_stack_color.isValid())
	{
		old_stack_color = m_color_stack;
	}

	// Repaint TTY with new colors
	QTextCursor tty_cursor = m_tty->textCursor();
	QTextCharFormat text_format = tty_cursor.charFormat();
	text_format.setForeground(gui::utils::get_label_color("tty_text"));
	tty_cursor.setCharFormat(text_format);
	m_tty->setTextCursor(tty_cursor);

	// Repaint log with new colors
	QString html = m_log->document()->toHtml();

	const QHash<int, QChar> log_chars
	{
		{ static_cast<int>(logs::level::always),  '-' },
		{ static_cast<int>(logs::level::fatal),   'F' },
		{ static_cast<int>(logs::level::error),   'E' },
		{ static_cast<int>(logs::level::todo),    'U' },
		{ static_cast<int>(logs::level::success), 'S' },
		{ static_cast<int>(logs::level::warning), 'W' },
		{ static_cast<int>(logs::level::notice),  '!' },
		{ static_cast<int>(logs::level::trace),   'T' }
	};

	const auto replace_color = [&](logs::level lvl)
	{
		const QString old_style = QStringLiteral("color:") + old_colors[static_cast<int>(lvl)].name() + QStringLiteral(";\">") + log_chars[static_cast<int>(lvl)];
		const QString new_style = QStringLiteral("color:") + m_color[static_cast<int>(lvl)].name() + QStringLiteral(";\">") + log_chars[static_cast<int>(lvl)];
		html.replace(old_style, new_style);
	};

	replace_color(logs::level::always);
	replace_color(logs::level::fatal);
	replace_color(logs::level::error);
	replace_color(logs::level::todo);
	replace_color(logs::level::success);
	replace_color(logs::level::warning);
	replace_color(logs::level::notice);
	replace_color(logs::level::trace);

	// Special case: stack
	const QString old_style = QStringLiteral("color:") + old_stack_color.name() + QStringLiteral(";\"> x");
	const QString new_style = QStringLiteral("color:") + m_color_stack.name() + QStringLiteral(";\"> x");
	html.replace(old_style, new_style);

	m_log->document()->setHtml(html);
}

void log_frame::UpdateUI()
{
	const auto start = steady_clock::now();

	// Check TTY logs
	while (const u64 size = std::max<s64>(0, g_tty_size.load() - (m_tty_file ? m_tty_file.pos() : 0)))
	{
		std::string buf;
		buf.resize(size);
		buf.resize(m_tty_file.read(&buf.front(), buf.size()));

		// Ignore control characters and greater/equal to 0x80
		buf.erase(std::remove_if(buf.begin(), buf.end(), [](s8 c) { return c <= 0x8 || c == 0x7F || (c >= 0xE && c <= 0x1F); }), buf.end());

		if (!buf.empty() && m_tty_act->isChecked())
		{
			std::stringstream buf_stream;
			buf_stream.str(buf);
			std::string buf_line;
			while (std::getline(buf_stream, buf_line))
			{
				// save old scroll bar state
				QScrollBar* sb = m_tty->verticalScrollBar();
				const int sb_pos = sb->value();
				const bool is_max = sb_pos == sb->maximum();

				// save old selection
				QTextCursor text_cursor{ m_tty->document() };
				const int sel_pos = text_cursor.position();
				int sel_start = text_cursor.selectionStart();
				int sel_end = text_cursor.selectionEnd();

				// clear selection or else it will get colorized as well
				text_cursor.clearSelection();

				const QString tty_text = QString::fromStdString(buf_line);

				// create counter suffix and remove recurring line if needed
				if (m_stack_tty)
				{
					text_cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);

					if (tty_text == m_old_tty_text)
					{
						text_cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
						text_cursor.insertText(tty_text % QStringLiteral(" x") % QString::number(++m_tty_counter));
					}
					else
					{
						m_tty_counter = 1;
						m_old_tty_text = tty_text;

						// write text to the end
						m_tty->setTextCursor(text_cursor);
						m_tty->appendPlainText(tty_text);
					}
				}
				else
				{
					// write text to the end
					m_tty->setTextCursor(text_cursor);
					m_tty->appendPlainText(tty_text);
				}

				// if we mark text from right to left we need to swap sides (start is always smaller than end)
				if (sel_pos < sel_end)
				{
					std::swap(sel_start, sel_end);
				}

				// reset old text cursor and selection
				text_cursor.setPosition(sel_start);
				text_cursor.setPosition(sel_end, QTextCursor::KeepAnchor);
				m_tty->setTextCursor(text_cursor);

				// set scrollbar to max means auto-scroll
				sb->setValue(is_max ? sb->maximum() : sb_pos);
			}
		}

		// Limit processing time
		if (steady_clock::now() >= start + 4ms || buf.empty()) break;
	}

	const auto font_start_tag = [](const QColor& color) -> const QString { return QStringLiteral("<font color = \"") % color.name() % QStringLiteral("\">"); };
	const QString font_start_tag_stack = "<font color = \"" % m_color_stack.name() % "\">";
	const QString font_end_tag = QStringLiteral("</font>");

	static constexpr auto escaped = [](QString& text)
	{
		return text.toHtmlEscaped().replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
	};

	// Check main logs
	while (auto* packet = s_gui_listener.get())
	{
		// Confirm log level
		if (packet->sev <= s_gui_listener.enabled)
		{
			QString text;
			switch (packet->sev)
			{
			case logs::level::always: text = QStringLiteral("- "); break;
			case logs::level::fatal: text = QStringLiteral("F "); break;
			case logs::level::error: text = QStringLiteral("E "); break;
			case logs::level::todo: text = QStringLiteral("U "); break;
			case logs::level::success: text = QStringLiteral("S "); break;
			case logs::level::warning: text = QStringLiteral("W "); break;
			case logs::level::notice: text = QStringLiteral("! "); break;
			case logs::level::trace: text = QStringLiteral("T "); break;
			}

			// Print UTF-8 text.
			text += qstr(packet->msg);

			// save old log state
			QScrollBar* sb = m_log->verticalScrollBar();
			const bool isMax = sb->value() == sb->maximum();
			const int sb_pos = sb->value();

			QTextCursor text_cursor = m_log->textCursor();
			const int sel_pos = text_cursor.position();
			int sel_start = text_cursor.selectionStart();
			int sel_end = text_cursor.selectionEnd();

			// clear selection or else it will get colorized as well
			text_cursor.clearSelection();

			// remove the new line because Qt's append adds a new line already.
			text.chop(1);

			// create counter suffix and remove recurring line if needed
			if (m_stack_log)
			{
				// add counter suffix if needed
				if (text == m_old_log_text)
				{
					text_cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
					text_cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
					text_cursor.insertHtml(font_start_tag(m_color[static_cast<int>(packet->sev)]) % escaped(text) % font_start_tag_stack % QStringLiteral(" x") % QString::number(++m_log_counter) % font_end_tag % font_end_tag);
				}
				else
				{
					m_log_counter = 1;
					m_old_log_text = text;

					m_log->setTextCursor(text_cursor);
					m_log->appendHtml(font_start_tag(m_color[static_cast<int>(packet->sev)]) % escaped(text) % font_end_tag);
				}
			}
			else
			{
				m_log->setTextCursor(text_cursor);
				m_log->appendHtml(font_start_tag(m_color[static_cast<int>(packet->sev)]) % escaped(text) % font_end_tag);
			}

			// if we mark text from right to left we need to swap sides (start is always smaller than end)
			if (sel_pos < sel_end)
			{
				std::swap(sel_start, sel_end);
			}

			// reset old text cursor and selection
			text_cursor.setPosition(sel_start);
			text_cursor.setPosition(sel_end, QTextCursor::KeepAnchor);
			m_log->setTextCursor(text_cursor);

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
		QKeyEvent* e = static_cast<QKeyEvent*>(event);
		if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_F)
		{
			if (m_find_dialog && m_find_dialog->isVisible())
				m_find_dialog->close();

			m_find_dialog.reset(new find_dialog(static_cast<QTextEdit*>(object), this));
		}
	}

	return QDockWidget::eventFilter(object, event);
}
