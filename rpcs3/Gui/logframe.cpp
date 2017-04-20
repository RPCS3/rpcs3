#include "logframe.h"

#include <stdafx.h>
#include "rpcs3_version.h"

#include <QMenu>
#include <QActionGroup>

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
		last->msg = fmt::format("RPCS3%s\n", rpcs3::version.to_string());

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

			if (msg.ch->name)
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

LogFrame::LogFrame(QWidget *parent) : QDockWidget(tr("Log"), parent)
{
	tabWidget = new QTabWidget;

	log = new QTextEdit(tabWidget);
	QPalette logPalette = log->palette();
	logPalette.setColor(QPalette::Base, Qt::black);
	log->setPalette(logPalette);
	log->setReadOnly(true);

	tty = new QTextEdit(tabWidget);
	QPalette ttyPalette = log->palette();
	ttyPalette.setColor(QPalette::Base, Qt::black);
	ttyPalette.setColor(QPalette::Text, Qt::white);
	tty->setPalette(ttyPalette);
	tty->setReadOnly(true);

	tabWidget->addTab(log, tr("Log"));
	tabWidget->addTab(tty, tr("TTY"));

	setWidget(tabWidget);

	// Open or create TTY.log
	tty_file.open(fs::get_config_dir() + "TTY.log", fs::read + fs::create);

	CreateAndConnectActions();
	log->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(log, &QWidget::customContextMenuRequested, this, &LogFrame::OpenLogContextMenu);

	// Check for updates every ~10 ms
	QTimer *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &LogFrame::UpdateUI);
	timer->start(10);

	// Update listener info
	// TODO: Use settings file
	s_gui_listener.enabled = logs::level::warning;
}

void LogFrame::CreateAndConnectActions()
{
	// I, for one, welcome our lambda overlord
	// It's either this or a signal mapper
	// Then, probably making a list of these actions so that it's easier to iterate to generate the mapper.
	auto l_initAct = [this](QAction* act, int logLevel)
	{
		act->setCheckable(true);
		
		// This sets the log level properly when the action is triggered.
		auto l_callback = [this, logLevel]() {
			s_gui_listener.enabled = static_cast<logs::level>(logLevel - id_log_level);
		};

		connect(act, &QAction::triggered, l_callback);
	};

	clearAct = new QAction(tr("Clear"), this);
	connect(clearAct, &QAction::triggered, log, &QTextEdit::clear);

	// Action groups make these actions mutually exclusive.
	logLevels = new QActionGroup(this);
	nothingAct = new QAction(tr("Nothing"), logLevels);
	fatalAct = new QAction(tr("Fatal"), logLevels);
	errorAct = new QAction(tr("Error"), logLevels);
	todoAct = new QAction(tr("Todo"), logLevels);
	successAct = new QAction(tr("Success"), logLevels);
	warningAct = new QAction(tr("Warning"), logLevels);
	noticeAct = new QAction(tr("Notice"), logLevels);
	traceAct = new QAction(tr("Trace"), logLevels);

	TTYAct = new QAction(tr("TTY"), this);

	l_initAct(nothingAct, id_log_level + 0);
	l_initAct(fatalAct, id_log_level + 1);
	l_initAct(errorAct, id_log_level + 2);
	l_initAct(todoAct, id_log_level + 3);
	l_initAct(successAct, id_log_level + 4);
	l_initAct(warningAct, id_log_level + 5);
	l_initAct(noticeAct, id_log_level + 6);
	l_initAct(traceAct, id_log_level + 7);

	// I don't think the action needs to be connected as its state is checked in updateui
	// (TODO: Make Settings)
	TTYAct->setCheckable(true);
	TTYAct->setChecked(true);

	// Warnings are default level so check it.
	warningAct->setChecked(true);
}


void LogFrame::UpdateUI()
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

	const auto start = steady_clock::now();

	// Check TTY logs

	while (const u64 size = std::min<u64>(buf.size(), tty_file.size() - tty_file.pos()))
	{
		QString& text = get_utf8(tty_file, size);

		// Hackily used the state of the check..  be better if I actually stored this value.
		if (TTYAct->isChecked())
		{
			text.chop(1); // remove newline since Qt automatically adds a newline.
			tty->append(text);
		}
		// Limit processing time
		if (steady_clock::now() >= start + 4ms || text.isEmpty()) break;
	}

	// Check main logs
	while (const auto packet = s_gui_listener.read->next.load())
	{
		// Confirm log level
		if (packet->sev <= s_gui_listener.enabled)
		{
			// Get text color
			QColor color;
			QString text;
			switch (packet->sev)
			{
			case logs::level::always: color = QColor(0x00, 0xFF, 0xFF); break; // Cyan
			case logs::level::fatal: text = "F "; color = QColor(0xFF, 0x00, 0xFF); break; // Fuchsia
			case logs::level::error: text = "E "; color = QColor(0xFF, 0x00, 0x00); break; // Red
			case logs::level::todo: text = "U "; color = QColor(0xFF, 0x60, 0x00); break; // Orange
			case logs::level::success: text = "S ";  color = QColor(0x00, 0xFF, 0x00); break; // Green
			case logs::level::warning: text = "W "; color = QColor(0xFF, 0xFF, 0x00); break; // Yellow
			case logs::level::notice: text = "! ";  color = QColor(0xFF, 0xFF, 0xFF); break; // White
			case logs::level::trace: text = "T ";  color = QColor(0x80, 0x80, 0x80); break; // Gray
			default: continue;
			}

			// Print UTF-8 text.
			text += QString::fromStdString(packet->msg);
			log->setTextColor(color);
			// remove the new line because Qt's append adds a new line already.
			text.chop(1);
			log->append(text);
		}

		// Drop packet
		s_gui_listener.pop();

		// Limit processing time
		if (steady_clock::now() >= start + 7ms) break;
	}
}

void LogFrame::OpenLogContextMenu(const QPoint& loc)
{
	QMenu* menu = log->createStandardContextMenu();
	menu->addAction(clearAct);
	menu->addSeparator();
	menu->addActions({ nothingAct, fatalAct, errorAct, todoAct, successAct, warningAct, noticeAct, traceAct });

	menu->addSeparator();
	menu->addAction(TTYAct);
	menu->exec(mapToGlobal(loc));
}

void LogFrame::closeEvent(QCloseEvent *event)
{
	QDockWidget::closeEvent(event);
	emit LogFrameClosed();
}
