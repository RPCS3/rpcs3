#include "stdafx.h"

#include "log_viewer.h"
#include "gui_settings.h"
#include "syntax_highlighter.h"
#include "config_checker.h"
#include "Crypto/unzip.h"

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QTextStream>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QMimeData>
#include <QScrollBar>
#include <QMessageBox>

#include <deque>
#include <map>

LOG_CHANNEL(gui_log, "GUI");

log_viewer::log_viewer(std::shared_ptr<gui_settings> gui_settings)
    : m_gui_settings(std::move(gui_settings))
{
	setWindowTitle(tr("Log Viewer"));
	setObjectName("log_viewer");
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_StyledBackground);
	setAcceptDrops(true);
	setMinimumSize(QSize(200, 150)); // seems fine on win 10
	resize(QSize(620, 395));

	m_path_last = m_gui_settings->GetValue(gui::fd_log_viewer).toString();
	m_show_timestamps = m_gui_settings->GetValue(gui::lv_show_timestamps).toBool();
	m_show_threads = m_gui_settings->GetValue(gui::lv_show_threads).toBool();
	m_log_levels = std::bitset<32>(m_gui_settings->GetValue(gui::lv_log_levels).toUInt());

	m_log_text = new QPlainTextEdit(this);
	m_log_text->setReadOnly(true);
	m_log_text->setContextMenuPolicy(Qt::CustomContextMenu);
	m_log_text->setWordWrapMode(QTextOption::NoWrap);
	m_log_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	m_log_text->installEventFilter(this);

	// m_log_text syntax highlighter
	m_log_highlighter = new LogHighlighter(m_log_text->document());

	QHBoxLayout* layout = new QHBoxLayout();
	layout->addWidget(m_log_text);

	setLayout(layout);

	connect(m_log_text, &QWidget::customContextMenuRequested, this, &log_viewer::show_context_menu);
}

void log_viewer::show_context_menu(const QPoint& pos)
{
	QMenu menu;
	QAction* clear  = new QAction(tr("&Clear"));
	QAction* copy   = new QAction(tr("&Copy"));
	QAction* open   = new QAction(tr("&Open log file"));
	QAction* save   = new QAction(tr("&Save filtered log"));
	QAction* filter = new QAction(tr("&Filter log"));
	QAction* config = new QAction(tr("&Check config"));

	QAction* timestamps = new QAction(tr("&Show Timestamps"));
	timestamps->setCheckable(true);
	timestamps->setChecked(m_show_timestamps);

	QAction* threads = new QAction(tr("&Show Threads"));
	threads->setCheckable(true);
	threads->setChecked(m_show_threads);

	QAction* last_actions_only = new QAction(tr("&Last actions only"));
	last_actions_only->setCheckable(true);
	last_actions_only->setChecked(m_last_actions_only);

	QActionGroup* log_level_acts = new QActionGroup(this);
	QAction* fatal_act = new QAction(tr("Fatal"), log_level_acts);
	QAction* error_act = new QAction(tr("Error"), log_level_acts);
	QAction* todo_act = new QAction(tr("Todo"), log_level_acts);
	QAction* success_act = new QAction(tr("Success"), log_level_acts);
	QAction* warning_act = new QAction(tr("Warning"), log_level_acts);
	QAction* notice_act = new QAction(tr("Notice"), log_level_acts);
	QAction* trace_act = new QAction(tr("Trace"), log_level_acts);
	log_level_acts->setExclusive(false);

	auto init_action = [this](QAction* act, logs::level logLevel)
	{
		act->setCheckable(true);
		act->setChecked(m_log_levels.test(static_cast<u32>(logLevel)));

		// This sets the log level properly when the action is triggered.
		connect(act, &QAction::triggered, this, [this, logLevel](bool checked)
		{
			m_log_levels.set(static_cast<u32>(logLevel), checked);
			m_gui_settings->SetValue(gui::lv_log_levels, ::narrow<u32>(m_log_levels.to_ulong()));
			filter_log();
		});
	};

	init_action(fatal_act, logs::level::fatal);
	init_action(error_act, logs::level::error);
	init_action(todo_act, logs::level::todo);
	init_action(success_act, logs::level::success);
	init_action(warning_act, logs::level::warning);
	init_action(notice_act, logs::level::notice);
	init_action(trace_act, logs::level::trace);

	menu.addAction(copy);
	menu.addSeparator();
	menu.addAction(open);
	menu.addSeparator();
	menu.addAction(save);
	menu.addSeparator();
	menu.addAction(config);
	menu.addSeparator();
	menu.addAction(filter);
	menu.addSeparator();
	menu.addAction(timestamps);
	menu.addSeparator();
	menu.addAction(threads);
	menu.addSeparator();
	menu.addAction(last_actions_only);
	menu.addSeparator();
	menu.addActions(log_level_acts->actions());
	menu.addSeparator();
	menu.addAction(clear);

	connect(copy, &QAction::triggered, this, [this]()
	{
		m_log_text->copy();
	});

	connect(clear, &QAction::triggered, this, [this]()
	{
		m_log_text->clear();
		m_full_log.clear();
	});

	connect(open, &QAction::triggered, this, [this]()
	{
		const QString file_path = QFileDialog::getOpenFileName(this, tr("Select log file"), m_path_last, tr("Log files (*.log *.gz);;All files (*.*)"));
		if (file_path.isEmpty())
			return;
		m_path_last = file_path;
		show_log();
	});

	connect(save, &QAction::triggered, this, [this]()
	{
		const QString file_path = QFileDialog::getSaveFileName(this, tr("Save to file"), m_path_last, tr("Log files (*.log *.gz);;All files (*.*)"));
		if (file_path.isEmpty())
			return;

		if (fs::file log_file; log_file.open(file_path.toStdString(), fs::rewrite))
		{
			const QByteArray bytes = m_log_text->toPlainText().toUtf8();

			if (file_path.endsWith(".gz"))
			{
				if (!zip(bytes.constData(), bytes.size(), log_file))
				{
					gui_log.error("Failed to zip filtered log to file '%s'", file_path);
					return;
				}
			}
			else
			{
				log_file.write(bytes.constData(), bytes.size());
			}

			gui_log.success("Exported filtered log to file '%s'", file_path);
		}
		else
		{
			gui_log.error("Failed to export filtered log to file '%s'", file_path);
		}
	});

	connect(config, &QAction::triggered, this, [this]()
	{
		config_checker* dlg = new config_checker(this, m_full_log, true);
		dlg->open();
	});

	connect(filter, &QAction::triggered, this, [this]()
	{
		m_filter_term = QInputDialog::getText(this, tr("Filter log"), tr("Enter text"), QLineEdit::EchoMode::Normal, m_filter_term);
		filter_log();
	});

	connect(threads, &QAction::toggled, this, [this](bool checked)
	{
		m_show_threads = checked;
		m_gui_settings->SetValue(gui::lv_show_threads, m_show_threads);
		filter_log();
	});

	connect(timestamps, &QAction::toggled, this, [this](bool checked)
	{
		m_show_timestamps = checked;
		m_gui_settings->SetValue(gui::lv_show_timestamps, m_show_timestamps);
		filter_log();
	});

	connect(last_actions_only, &QAction::toggled, this, [this](bool checked)
	{
		m_last_actions_only = checked;
		filter_log();
	});

	const auto obj = qobject_cast<QPlainTextEdit*>(sender());

	QPoint origin;

	if (obj == m_log_text)
	{
		origin = m_log_text->viewport()->mapToGlobal(pos);
	}
	else
	{
		origin = mapToGlobal(pos);
	}

	menu.exec(origin);
}

void log_viewer::show_log()
{
	if (m_path_last.isEmpty())
	{
		return;
	}

	m_full_log.clear();
	m_log_text->clear();
	m_log_text->setPlainText(tr("Loading file..."));
	QApplication::processEvents();

	bool failed = false;

	if (m_path_last.endsWith(".gz"))
	{
		if (fs::file file{m_path_last.toStdString()})
		{
			const std::vector<u8> decompressed = unzip(file.to_vector<u8>());
			m_full_log = QString::fromUtf8(reinterpret_cast<const char*>(decompressed.data()), decompressed.size());
		}
		else
		{
			failed = true;
		}
	}
	else if (QFile file(m_path_last); file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		// TODO: Due to a bug in Qt 6.5.2 QTextStream::readAll is ridiculously slow to the point where it gets stuck on large files.
		//       In Qt 5.15.2 this was much faster than QFile::readAll. Use QTextStream again once this bug is fixed upstream.
		//QTextStream stream(&file);
		//m_full_log = stream.readAll();
		m_full_log = file.readAll();
	}
	else
	{
		failed = true;
	}

	if (failed)
	{
		gui_log.error("log_viewer: Failed to open %s", m_path_last);
		m_log_text->setPlainText(tr("Failed to open '%0'").arg(m_path_last));
	}
	else
	{
		m_gui_settings->SetValue(gui::fd_log_viewer, m_path_last);
		m_full_log.replace('\0', '0');
	}

	filter_log();
}

void log_viewer::set_text_and_keep_position(const QString& text)
{
	m_log_text->setPlainText(tr("Pasting..."));
	QApplication::processEvents();

	const int pos = m_log_text->verticalScrollBar()->value();
	m_log_text->setPlainText(text);
	m_log_text->verticalScrollBar()->setValue(pos);
}

void log_viewer::filter_log()
{
	if (m_full_log.isEmpty())
	{
		set_text_and_keep_position(m_full_log);
		return;
	}

	m_log_text->setPlainText(tr("Filtering..."));

	std::vector<QString> excluded_log_levels;
	if (!m_log_levels.test(static_cast<u32>(logs::level::fatal)))   excluded_log_levels.push_back("·F ");
	if (!m_log_levels.test(static_cast<u32>(logs::level::error)))   excluded_log_levels.push_back("·E ");
	if (!m_log_levels.test(static_cast<u32>(logs::level::todo)))    excluded_log_levels.push_back("·U ");
	if (!m_log_levels.test(static_cast<u32>(logs::level::success))) excluded_log_levels.push_back("·S ");
	if (!m_log_levels.test(static_cast<u32>(logs::level::warning))) excluded_log_levels.push_back("·W ");
	if (!m_log_levels.test(static_cast<u32>(logs::level::notice)))  excluded_log_levels.push_back("·! ");
	if (!m_log_levels.test(static_cast<u32>(logs::level::trace)))   excluded_log_levels.push_back("·T ");

	if (m_filter_term.isEmpty() && excluded_log_levels.empty() && m_show_timestamps && m_show_threads && !m_last_actions_only)
	{
		set_text_and_keep_position(m_full_log);
		return;
	}

	QString result;
	QTextStream stream(&m_full_log);
	const QRegularExpression thread_regexp("\\{.*\\} ");
	const QRegularExpression timestamp_regexp("\\d?\\d:\\d\\d:\\d\\d\\.\\d\\d\\d\\d\\d\\d ");

	const auto add_line = [this, &result, &excluded_log_levels, &timestamp_regexp, &thread_regexp](QString& line)
	{
		bool exclude_line = false;

		for (const QString& log_level_prefix : excluded_log_levels)
		{
			if (line.startsWith(log_level_prefix))
			{
				exclude_line = true;
				break;
			}
		}

		if (exclude_line)
		{
			return;
		}

		if (m_filter_term.isEmpty() || line.contains(m_filter_term))
		{
			if (line.isEmpty())
			{
				result += "\n";
				return;
			}

			if (!m_show_timestamps)
			{
				line.remove(timestamp_regexp);
			}

			if (!m_show_threads)
			{
				line.remove(thread_regexp);
			}

			if (!line.isEmpty())
			{
				result += line + "\n";
			}
		}
	};

	if (m_last_actions_only)
	{
		if (const int start_pos = m_full_log.lastIndexOf("LDR: Used configuration:"); start_pos >= 0)
		{
			if (!stream.seek(start_pos)) // TODO: is this correct ?
			{
				gui_log.error("Log viewer failed to seek to pos %d of log.", start_pos);
			}

			std::map<QString, std::deque<QString>> all_thread_actions;

			for (QString line = stream.readLine(); !line.isNull(); line = stream.readLine())
			{
				if (const QRegularExpressionMatch match = thread_regexp.match(line); match.hasMatch())
				{
					if (const QString thread_name = match.captured(); !thread_name.isEmpty())
					{
						std::deque<QString>& actions = all_thread_actions[thread_name];
						actions.push_back(line);

						if (actions.size() > 10)
						{
							actions.pop_front();
						}
					}
				}
			}

			for (auto& [thread_name, actions] : all_thread_actions)
			{
				for (QString& line : actions)
				{
					add_line(line);
				}
			}

			set_text_and_keep_position(result);
			return;
		}

		QMessageBox::information(this, tr("Ooops!"), tr("Cannot find any game boot!"));
		// Pass through to regular log filter
	}

	if (!stream.seek(0))
	{
		gui_log.error("Log viewer failed to seek to beginning of log.");
	}

	for (QString line = stream.readLine(); !line.isNull(); line = stream.readLine())
	{
		add_line(line);
	};

	set_text_and_keep_position(result);
}

bool log_viewer::is_valid_file(const QMimeData& md, bool save)
{
	const QList<QUrl> urls = md.urls();

	if (urls.count() > 1)
	{
		return false;
	}

	const QString suffix = QFileInfo(urls[0].fileName()).suffix().toLower();

	if (suffix == "log" || suffix == "gz")
	{
		if (save)
		{
			m_path_last = urls[0].toLocalFile();
		}
		return true;
	}
	return false;
}

void log_viewer::dropEvent(QDropEvent* ev)
{
	if (is_valid_file(*ev->mimeData(), true))
	{
		show_log();
	}
}

void log_viewer::dragEnterEvent(QDragEnterEvent* ev)
{
	if (is_valid_file(*ev->mimeData()))
	{
		ev->accept();
	}
}

void log_viewer::dragMoveEvent(QDragMoveEvent* ev)
{
	if (is_valid_file(*ev->mimeData()))
	{
		ev->accept();
	}
}

void log_viewer::dragLeaveEvent(QDragLeaveEvent* ev)
{
	ev->accept();
}

bool log_viewer::eventFilter(QObject* object, QEvent* event)
{
	if (object != m_log_text)
	{
		return QWidget::eventFilter(object, event);
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

	return QWidget::eventFilter(object, event);
}
