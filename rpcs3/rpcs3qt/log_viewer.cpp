#include "stdafx.h"

#include "log_viewer.h"
#include "gui_settings.h"
#include "syntax_highlighter.h"
#include "find_dialog.h"

#include <QApplication>
#include <QMenu>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QMimeData>

LOG_CHANNEL(gui_log, "GUI");

[[maybe_unused]] constexpr auto qstr = QString::fromStdString;

inline std::string sstr(const QString& _in)
{
	return _in.toStdString();
}

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
	QAction* clear = new QAction(tr("&Clear"));
	QAction* open  = new QAction(tr("&Open log file"));

	menu.addAction(open);
	menu.addSeparator();
	menu.addAction(clear);

	connect(clear, &QAction::triggered, [this]()
	{
		m_log_text->clear();
	});

	connect(open, &QAction::triggered, [this]()
	{
		const QString file_path = QFileDialog::getOpenFileName(this, tr("Select log file"), m_path_last, tr("Log files (*.log);;"));
		if (file_path.isEmpty())
			return;
		m_path_last = file_path;
		show_log();
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

void log_viewer::show_log() const
{
	if (m_path_last.isEmpty())
	{
		return;
	}

	m_log_text->clear();
	m_log_text->setPlainText(tr("Loading file..."));
	QApplication::processEvents();

	if (QFile file(m_path_last);
		file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		m_gui_settings->SetValue(gui::fd_log_viewer, m_path_last);

		QTextStream stream(&file);
		m_log_text->setPlainText(stream.readAll());
		file.close();
	}
	else
	{
		gui_log.error("log_viewer: Failed to open %s", sstr(m_path_last));
		m_log_text->setPlainText(tr("Failed to open '%0'").arg(m_path_last));
	}
}

bool log_viewer::is_valid_file(const QMimeData& md, bool save)
{
	const QList<QUrl> urls = md.urls();

	if (urls.count() > 1)
	{
		return false;
	}

	const QString suffix = QFileInfo(urls[0].fileName()).suffix().toLower();

	if (suffix == "log")
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
		if (e && e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_F)
		{
			if (m_find_dialog && m_find_dialog->isVisible())
				m_find_dialog->close();

			m_find_dialog.reset(new find_dialog(static_cast<QTextEdit*>(object), this));
		}
	}

	return QWidget::eventFilter(object, event);
}
