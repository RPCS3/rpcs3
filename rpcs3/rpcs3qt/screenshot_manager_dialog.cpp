#include "stdafx.h"
#include "screenshot_manager_dialog.h"
#include "screenshot_preview.h"
#include "screenshot_item.h"
#include "flow_widget.h"
#include "qt_utils.h"
#include "Utilities/File.h"
#include "Emu/system_utils.hpp"

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QScreen>
#include <QVBoxLayout>
#include <QtConcurrent>

LOG_CHANNEL(gui_log, "GUI");

screenshot_manager_dialog::screenshot_manager_dialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle(tr("Screenshots"));
	setAttribute(Qt::WA_DeleteOnClose);

	m_icon_size = QSize(160, 90);
	m_flow_widget = new flow_widget(this);
	m_flow_widget->setObjectName("flow_widget");

	m_placeholder = QPixmap(m_icon_size);
	m_placeholder.fill(Qt::gray);

	connect(this, &screenshot_manager_dialog::signal_icon_preview, this, &screenshot_manager_dialog::show_preview);
	connect(this, &screenshot_manager_dialog::signal_entry_parsed, this, &screenshot_manager_dialog::add_entry);

	QVBoxLayout* layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_flow_widget);
	setLayout(layout);

	resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
}

screenshot_manager_dialog::~screenshot_manager_dialog()
{
	m_abort_parsing = true;
	gui::utils::stop_future_watcher(m_parsing_watcher, true);
}

void screenshot_manager_dialog::add_entry(const QString& path)
{
	screenshot_item* item = new screenshot_item(m_flow_widget);
	ensure(item->label);
	item->setToolTip(path);
	item->installEventFilter(this);
	item->label->setPixmap(m_placeholder);
	item->icon_path = path;
	item->icon_size = m_icon_size;
	connect(item, &screenshot_item::signal_icon_update, this, &screenshot_manager_dialog::update_icon);

	m_flow_widget->add_widget(item);
}

void screenshot_manager_dialog::show_preview(const QString& path)
{
	screenshot_preview* preview = new screenshot_preview(path);
	preview->show();
}

void screenshot_manager_dialog::update_icon(const QPixmap& pixmap)
{
	if (screenshot_item* item = static_cast<screenshot_item*>(QObject::sender()))
	{
		if (item->label)
		{
			item->label->setPixmap(pixmap);
		}
	}
}

void screenshot_manager_dialog::reload()
{
	m_abort_parsing = true;
	gui::utils::stop_future_watcher(m_parsing_watcher, true);

	const std::string screenshot_path_qt   = fs::get_config_dir() + "screenshots/";
	const std::string screenshot_path_cell = rpcs3::utils::get_hdd0_dir() + "/photo/";

	m_flow_widget->clear();
	m_abort_parsing = false;
	m_parsing_watcher.setFuture(QtConcurrent::map(m_parsing_threads, [this, screenshot_path_qt, screenshot_path_cell](int index)
	{
		if (index != 0)
		{
			return;
		}

		const QStringList filter{ QStringLiteral("*.png") };

		for (const std::string& path : { screenshot_path_qt, screenshot_path_cell })
		{
			if (m_abort_parsing)
			{
				return;
			}

			if (path.empty())
			{
				gui_log.error("Screenshot manager: Trying to load screenshots from empty path!");
				continue;
			}

			QDirIterator dir_iter(QString::fromStdString(path), filter, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

			while (dir_iter.hasNext() && !m_abort_parsing)
			{
				Q_EMIT signal_entry_parsed(dir_iter.next());
			}
		}
	}));
}

void screenshot_manager_dialog::showEvent(QShowEvent* event)
{
	QDialog::showEvent(event);
	reload();
}

bool screenshot_manager_dialog::eventFilter(QObject* watched, QEvent* event)
{
	if (event && event->type() == QEvent::MouseButtonDblClick && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
	{
		if (screenshot_item* item = static_cast<screenshot_item*>(watched))
		{
			Q_EMIT signal_icon_preview(item->icon_path);
			return true;
		}
	}

	return false;
}
