#include "log_frame.h"
#include "qt_utils.h"
#include "gui_settings.h"
#include "hex_validator.h"
#include "memory_viewer_panel.h"

#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Utilities/lockless.h"
#include "util/asm.hpp"

#include <QtConcurrent>
#include <QMenu>
#include <QMessageBox>
#include <QActionGroup>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QTimer>
#include <QStringBuilder>

#include <deque>
#include <mutex>

LOG_CHANNEL(sys_log, "SYS");

extern fs::file g_tty;
extern atomic_t<s64> g_tty_size;
extern std::array<std::deque<std::string>, 16> g_tty_input;
extern std::mutex g_tty_mutex;
extern bool g_log_all_errors;

struct gui_listener : logs::listener
{
	atomic_t<logs::level> enabled{logs::level{0xff}};

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

	void log(u64 stamp, const logs::message& msg, std::string_view prefix, std::string_view text) override
	{
		Q_UNUSED(stamp)

		if (msg <= enabled)
		{
			packet_t p,* _new = &p;
			_new->sev = msg;

			if ((msg == logs::level::fatal || show_prefix) && !prefix.empty())
			{
				_new->msg += "{";
				_new->msg += prefix;
				_new->msg += "} ";
			}

			if (msg->name && '\0' != *msg->name)
			{
				_new->msg += msg->name;
				_new->msg += msg == logs::level::todo ? " TODO: " : ": ";
			}
			else if (msg == logs::level::todo)
			{
				_new->msg += "TODO: ";
			}

			_new->msg += text;

			queue.push<false>(std::move(p));
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

	void clear()
	{
		pending = lf_queue_slice<packet_t>();
		queue.pop_all();
	}
};

// GUI Listener instance
static gui_listener s_gui_listener;

log_frame::log_frame(std::shared_ptr<gui_settings> _gui_settings, QWidget* parent)
	: custom_dock_widget(tr("Log"), parent), m_gui_settings(std::move(_gui_settings))
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
		m_tty_input->setPlaceholderText(tr("All user channels"));
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
	m_tty_file.open(fs::get_log_dir() + "TTY.log", fs::read + fs::create);

	CreateAndConnectActions();
	LoadSettings();

	m_timer = new QTimer(this);
	connect(m_timer, &QTimer::timeout, this, &log_frame::UpdateUI);
}

void log_frame::show_disk_usage(const std::vector<std::pair<std::string, u64>>& vfs_disk_usage, u64 cache_disk_usage)
{
	QString text;
	u64 tot_data_size = 0;

	for (const auto& [dev, data_size] : vfs_disk_usage)
	{
		text += tr("\n    %0: %1").arg(QString::fromStdString(dev)).arg(gui::utils::format_byte_size(data_size));
		tot_data_size += data_size;
	}

	if (!text.isEmpty())
	{
		text = tr("\n  VFS disk usage: %0%1").arg(gui::utils::format_byte_size(tot_data_size)).arg(text);
	}

	text += tr("\n  Cache disk usage: %0").arg(gui::utils::format_byte_size(cache_disk_usage));

	sys_log.success("%s", text);
	QMessageBox::information(this, tr("Disk usage"), text);
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
		m_old_log_text.clear();
		m_log->clear();
		s_gui_listener.clear();
	});

	m_clear_tty_act = new QAction(tr("Clear"), this);
	connect(m_clear_tty_act, &QAction::triggered, [this]()
	{
		m_old_tty_text.clear();
		m_tty->clear();
	});

	m_show_disk_usage_act = new QAction(tr("Show Disk Usage"), this);
	connect(m_show_disk_usage_act, &QAction::triggered, [this]()
	{
		if (m_disk_usage_future.isRunning())
		{
			return; // Still running the last request
		}

		m_disk_usage_future = QtConcurrent::run([this]()
		{
			const std::vector<std::pair<std::string, u64>> vfs_disk_usage = rpcs3::utils::get_vfs_disk_usage();
			const u64 cache_disk_usage = rpcs3::utils::get_cache_disk_usage();

			Emu.CallFromMainThread([this, vfs_disk_usage, cache_disk_usage]()
			{
				show_disk_usage(vfs_disk_usage, cache_disk_usage);
			}, nullptr, false);
		});
	});

	m_perform_goto_on_debugger = new QAction(tr("Go-To On The Debugger"), this);
	connect(m_perform_goto_on_debugger, &QAction::triggered, [this]()
	{
		QPlainTextEdit* pte = (m_tabWidget->currentIndex() == 1 ? m_tty : m_log);
		Q_EMIT PerformGoToOnDebugger(pte->textCursor().selectedText(), true);
	});

	m_perform_show_in_mem_viewer = new QAction(tr("Show in Memory Viewer"), this);
	connect(m_perform_show_in_mem_viewer, &QAction::triggered, this, [this]()
	{
		const QPlainTextEdit* pte = (m_tabWidget->currentIndex() == 1 ? m_tty : m_log);
		const QString selected = pte->textCursor().selectedText();
		u64 pc = 0;
		if (!parse_hex_qstring(selected, &pc))
		{
			QMessageBox::critical(this, tr("Invalid Hex"), tr("“%0” is not a valid 32-bit hex value.").arg(selected));
			return;
		}

		memory_viewer_panel::ShowAtPC(static_cast<u32>(pc));
	});

	m_perform_goto_thread_on_debugger = new QAction(tr("Show Thread On The Debugger"), this);
	connect(m_perform_goto_thread_on_debugger, &QAction::triggered, [this]()
	{
		QPlainTextEdit* pte = (m_tabWidget->currentIndex() == 1 ? m_tty : m_log);
		Q_EMIT PerformGoToOnDebugger(pte->textCursor().selectedText(), false);
	});

	m_stack_act_tty = new QAction(tr("Stack Mode (TTY)"), this);
	m_stack_act_tty->setCheckable(true);
	connect(m_stack_act_tty, &QAction::toggled, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::l_stack_tty, checked);
		m_stack_tty = checked;
	});

	m_ansi_act_tty = new QAction(tr("ANSI Code (TTY)"), this);
	m_ansi_act_tty->setCheckable(true);
	connect(m_ansi_act_tty, &QAction::toggled, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::l_ansi_code, checked);
		m_ansi_tty = checked;
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

	m_stack_act_err = new QAction(tr("Stack Cell Errors"), this);
	m_stack_act_err->setCheckable(true);
	connect(m_stack_act_err, &QAction::toggled, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::l_stack_err, checked);
		g_log_all_errors = !checked;
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
		menu->addAction(m_show_disk_usage_act);
		menu->addSeparator();
		menu->addAction(m_perform_goto_on_debugger);
		menu->addAction(m_perform_goto_thread_on_debugger);
		menu->addAction(m_perform_show_in_mem_viewer);

		std::shared_ptr<bool> goto_signal_accepted = std::make_shared<bool>(false);
		Q_EMIT PerformGoToOnDebugger("", true, true, goto_signal_accepted);
		m_perform_goto_on_debugger->setEnabled(m_log->textCursor().hasSelection() && *goto_signal_accepted);
		m_perform_goto_thread_on_debugger->setEnabled(m_log->textCursor().hasSelection() && *goto_signal_accepted);
		m_perform_show_in_mem_viewer->setEnabled(m_log->textCursor().hasSelection() && *goto_signal_accepted);
		m_perform_goto_on_debugger->setToolTip(tr("Jump to the selected hexadecimal address from the log text on the debugger."));
		m_perform_goto_thread_on_debugger->setToolTip(tr("Show the thread that corresponds to the thread ID from the log text on the debugger."));
		m_perform_show_in_mem_viewer->setToolTip(tr("Jump to the selected hexadecimal address from the log text on the memory viewer."));

		menu->addSeparator();
		menu->addActions(m_log_level_acts->actions());
		menu->addSeparator();
		menu->addAction(m_stack_act_log);
		menu->addAction(m_stack_act_err);
		menu->addAction(m_show_prefix_act);
		menu->exec(m_log->viewport()->mapToGlobal(pos));
	});

	connect(m_tty, &QWidget::customContextMenuRequested, [this](const QPoint& pos)
	{
		QMenu* menu = m_tty->createStandardContextMenu();
		menu->addAction(m_clear_tty_act);
		menu->addAction(m_perform_goto_on_debugger);
		menu->addAction(m_perform_show_in_mem_viewer);

		std::shared_ptr<bool> goto_signal_accepted = std::make_shared<bool>(false);
		Q_EMIT PerformGoToOnDebugger("", false, true, goto_signal_accepted);
		m_perform_goto_on_debugger->setEnabled(m_tty->textCursor().hasSelection() && *goto_signal_accepted);
		m_perform_show_in_mem_viewer->setEnabled(m_tty->textCursor().hasSelection() && *goto_signal_accepted);
		m_perform_goto_on_debugger->setToolTip(tr("Jump to the selected hexadecimal address from the TTY text on the debugger."));
		m_perform_show_in_mem_viewer->setToolTip(tr("Jump to the selected hexadecimal address from the TTY text on the memory viewer."));

		menu->addSeparator();
		menu->addAction(m_tty_act);
		menu->addAction(m_stack_act_tty);
		menu->addAction(m_ansi_act_tty);
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
}

void log_frame::LoadSettings()
{
	SetLogLevel(m_gui_settings->GetLogLevel());
	SetTTYLogging(m_gui_settings->GetValue(gui::l_tty).toBool());
	m_stack_log = m_gui_settings->GetValue(gui::l_stack).toBool();
	m_stack_tty = m_gui_settings->GetValue(gui::l_stack_tty).toBool();
	m_ansi_tty = m_gui_settings->GetValue(gui::l_ansi_code).toBool();
	g_log_all_errors = !m_gui_settings->GetValue(gui::l_stack_err).toBool();
	m_stack_act_log->setChecked(m_stack_log);
	m_stack_act_tty->setChecked(m_stack_tty);
	m_ansi_act_tty->setChecked(m_ansi_tty);
	m_stack_act_err->setChecked(!g_log_all_errors);

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

	// Note: There's an issue where the scrollbar value won't be set to max if we start the log frame too early,
	//       so let's delay the timer until we load the settings from the main window for the first time.
	if (m_timer && !m_timer->isActive())
	{
		// Check for updates every ~10 ms
		m_timer->start(10);
	}
}

void log_frame::RepaintTextColors()
{
	// Backup old colors
	std::vector<QColor> old_colors = m_color;
	QColor old_stack_color = m_color_stack;

	const QColor color = gui::utils::get_foreground_color();

	// Get text color. Do this once to prevent possible slowdown
	m_color.clear();
	m_color.push_back(gui::utils::get_label_color("log_level_always", Qt::darkCyan, Qt::cyan));
	m_color.push_back(gui::utils::get_label_color("log_level_fatal", Qt::darkMagenta, Qt::magenta));
	m_color.push_back(gui::utils::get_label_color("log_level_error", Qt::red, Qt::red));
	m_color.push_back(gui::utils::get_label_color("log_level_todo", Qt::darkYellow, Qt::darkYellow));
	m_color.push_back(gui::utils::get_label_color("log_level_success", Qt::darkGreen, Qt::green));
	m_color.push_back(gui::utils::get_label_color("log_level_warning", Qt::darkYellow, Qt::darkYellow));
	m_color.push_back(gui::utils::get_label_color("log_level_notice", color, color));
	m_color.push_back(gui::utils::get_label_color("log_level_trace", color, color));

	m_color_stack = gui::utils::get_label_color("log_stack", color, color);

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
	text_format.setForeground(gui::utils::get_label_color("tty_text", color, color));
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
	const std::chrono::time_point start = steady_clock::now();
	const std::chrono::time_point tty_timeout = start + 4ms;
	const std::chrono::time_point log_timeout = start + 7ms;

	// Check TTY logs
	if (u64 size = std::max<s64>(0, m_tty_file ? (g_tty_size.load() - m_tty_file.pos()) : 0))
	{
		if (m_tty_act->isChecked())
		{
			m_tty_buf.resize(std::min<u64>(size, m_tty_limited_read ? m_tty_limited_read : usz{umax}));
			m_tty_buf.resize(m_tty_file.read(&m_tty_buf.front(), m_tty_buf.size()));
			m_tty_limited_read = 0;

			usz str_index = 0;
			std::string buf_line;

			while (str_index < m_tty_buf.size())
			{
				buf_line.assign(std::string_view(m_tty_buf).substr(str_index, m_tty_buf.find_first_of('\n', str_index) - str_index));
				str_index += buf_line.size() + 1;

				// Ignore control characters and greater/equal to 0x80
				buf_line.erase(std::remove_if(buf_line.begin(), buf_line.end(), [](s8 c) { return c <= 0x8 || c == 0x7F || (c >= 0xE && c <= 0x1F); }), buf_line.end());

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

				QString tty_text;

				if (m_ansi_tty)
				{
					tty_text = QString::fromStdString(buf_line);
				}
				else
				{
					// Strip ANSI color code
					static const QRegularExpression ansi_color_code("\\[\\d+;\\d+(;\\d+)?m|\\[([0-9]{1,2}(;[0-9]{1,2})?)?[m|K]");
					tty_text = QString::fromStdString(buf_line).remove(ansi_color_code);
				}

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

				// Limit processing time
				if (steady_clock::now() >= tty_timeout)
				{
					const s64 back = ::narrow<s64>(str_index) - ::narrow<s64>(m_tty_buf.size());
					ensure(back <= 1);

					if (back < 0)
					{
						// If more than two thirds of the buffer are unprocessed make the next fs::file::read read only that
						// This is because reading is also costly on performance, and if we already know half of that takes more time than our limit..
						const usz third_size = utils::aligned_div(m_tty_buf.size(), 3);

						if (back <= -16384 && static_cast<usz>(0 - back) >= third_size * 2)
						{
							// This only really works if there is a newline somewhere
							const usz known_term = std::string_view(m_tty_buf).substr(str_index, str_index * 2).find_last_of('\n', str_index * 2 - 4096);

							if (known_term != umax)
							{
								m_tty_limited_read = known_term + 1 - str_index;
							}
						}

						// Revert unprocessed reads
						m_tty_file.seek(back, fs::seek_cur);
					}

					break;
				}
			}
		}
		else
		{
			// Advance in position without printing
			m_tty_file.seek(size, fs::seek_cur);
			m_tty_limited_read = 0;
		}
	}

	const auto font_start_tag = [](const QColor& color) -> const QString { return QStringLiteral("<font color = \"") % color.name() % QStringLiteral("\">"); };
	const QString font_start_tag_stack = "<font color = \"" % m_color_stack.name() % "\">";
	static const QString font_end_tag = QStringLiteral("</font>");

	static constexpr auto escaped = [](const QString& text, QString&& storage) -> const QString&
	{
		const qsizetype nline = text.indexOf(QChar('\n'));
		const qsizetype spaces = text.indexOf(QStringLiteral("  "));
		const qsizetype html = std::max<qsizetype>({ text.indexOf(QChar('<')), text.indexOf(QChar('>')), text.indexOf(QChar('&')), text.indexOf(QChar('\"')) });

		const qsizetype pos = std::max<qsizetype>({ html, nline, spaces });

		if (pos < 0)
		{
			// Nothing to change, do not create copies of the string
			return text;
		}

		// Allow to return reference of new string by using temporary storage provided by argument
		storage = html < 0 ? text : text.toHtmlEscaped();

		if (nline >= 0)
		{
			storage.replace(QChar('\n'), QStringLiteral("<br/>"));
		}

		if (spaces >= 0)
		{
			storage.replace(QChar::Space, QChar::Nbsp);
		}

		return storage;
	};

	// Preserve capacity
	m_log_text.resize(0);

	// Handle a common case in which we may need to override the previous repetition count
	bool is_first_rep = m_stack_log;
	usz first_rep_counter = m_log_counter;

	// Batch output of multiple lines if possible (optimization)
	auto flush = [&]()
	{
		if (m_log_text.isEmpty() && !is_first_rep)
		{
			return;
		}

		// save old log state
		QScrollBar* sb = m_log->verticalScrollBar();
		const bool is_max = sb->value() == sb->maximum();
		const int sb_pos = sb->value();

		QTextCursor text_cursor = m_log->textCursor();
		const int sel_pos = text_cursor.position();
		int sel_start = text_cursor.selectionStart();
		int sel_end = text_cursor.selectionEnd();

		// clear selection or else it will get colorized as well
		text_cursor.clearSelection();

		m_log->setTextCursor(text_cursor);

		if (is_first_rep)
		{
			// Overwrite existing repetition counter in our text document.
			ensure(first_rep_counter > m_log_counter); // Anything else is a bug

			text_cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);

			if (m_log_counter > 1)
			{
				constexpr int size_of_x = 2; // " x"
				const int size_of_number = static_cast<int>(QString::number(m_log_counter).size());

				text_cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, size_of_x + size_of_number);
			}

			text_cursor.insertHtml(font_start_tag_stack % QStringLiteral("&nbsp;x") % QString::number(first_rep_counter) % font_end_tag);
		}
		else
		{
			// Insert both messages and repetition prefix (append is more optimized than concatenation)
			m_log_text += font_end_tag;

			if (m_log_counter > 1)
			{
				m_log_text += font_start_tag_stack;
				m_log_text += QStringLiteral(" x");
				m_log_text += QString::number(m_log_counter);
				m_log_text += font_end_tag;
			}

			m_log->appendHtml(m_log_text);
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
		sb->setValue(is_max ? sb->maximum() : sb_pos);
		m_log_text.clear();
	};

	const auto patch_first_stacked_message = [&]() -> bool
	{
		if (is_first_rep)
		{
			const bool needs_update = first_rep_counter > m_log_counter;

			if (needs_update)
			{
				// Update the existing stack suffix.
				flush();
			}

			is_first_rep = false;
			m_log_counter = first_rep_counter;

			return needs_update;
		}

		return false;
	};

	// Check main logs
	while (auto* packet = s_gui_listener.get())
	{
		// Confirm log level
		if (packet->sev <= s_gui_listener.enabled)
		{
			// Check if we can stack this log message.
			if (m_stack_log && m_old_log_level == packet->sev && packet->msg == m_old_log_text)
			{
				if (is_first_rep)
				{
					// Keep tracking the old stack suffix count as long as the last known message keeps repeating.
					first_rep_counter++;
				}
				else
				{
					m_log_counter++;
				}

				s_gui_listener.pop();

				if (steady_clock::now() >= log_timeout)
				{
					// Must break eventually
					break;
				}

				continue;
			}

			// Add/update counter suffix if needed. Try not to hold too much data at a time so the frame content will be updated frequently.
			if (!patch_first_stacked_message() && (m_log_counter > 1 || m_log_text.size() > 0x1000))
			{
				flush();
			}

			if (m_log_text.isEmpty())
			{
				m_old_log_level = packet->sev;
				m_log_text += font_start_tag(m_color[static_cast<int>(m_old_log_level)]);
			}
			else
			{
				if (packet->sev != m_old_log_level)
				{
					flush();
					m_old_log_level = packet->sev;
					m_log_text += font_start_tag(m_color[static_cast<int>(m_old_log_level)]);
				}
				else
				{
					m_log_text += QStringLiteral("<br/>");
				}
			}

			switch (packet->sev)
			{
			case logs::level::always: m_log_text += QStringLiteral("- "); break;
			case logs::level::fatal: m_log_text += QStringLiteral("F "); break;
			case logs::level::error: m_log_text += QStringLiteral("E "); break;
			case logs::level::todo: m_log_text += QStringLiteral("U "); break;
			case logs::level::success: m_log_text += QStringLiteral("S "); break;
			case logs::level::warning: m_log_text += QStringLiteral("W "); break;
			case logs::level::notice: m_log_text += QStringLiteral("! "); break;
			case logs::level::trace: m_log_text += QStringLiteral("T "); break;
			}

			// Print UTF-8 text.
			m_log_text += escaped(QString::fromStdString(packet->msg), QString{});

			if (m_stack_log)
			{
				m_log_counter = 1;
				m_old_log_text = std::move(packet->msg);
			}
		}

		// Drop packet
		s_gui_listener.pop();

		// Limit processing time
		if (steady_clock::now() >= log_timeout) break;
	}

	if (!patch_first_stacked_message())
	{
		flush();
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
		if (e && !e->isAutoRepeat() && e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_F)
		{
			if (m_find_dialog && m_find_dialog->isVisible())
				m_find_dialog->close();

			m_find_dialog.reset(new find_dialog(static_cast<QPlainTextEdit*>(object), this));
		}
	}

	return QDockWidget::eventFilter(object, event);
}
