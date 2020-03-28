#include "screenshot_manager_dialog.h"
#include "screenshot_preview.h"
#include "qt_utils.h"
#include "Utilities/File.h"

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QListWidget>
#include <QScreen>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QtConcurrent>

screenshot_manager_dialog::screenshot_manager_dialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle(tr("Screenshots"));
	setAttribute(Qt::WA_DeleteOnClose);

	m_icon_size = QSize(160, 90);

	m_grid = new QListWidget(this);
	m_grid->setViewMode(QListWidget::IconMode);
	m_grid->setMovement(QListWidget::Static);
	m_grid->setResizeMode(QListWidget::Adjust);
	m_grid->setIconSize(m_icon_size);
	m_grid->setGridSize(m_icon_size + QSize(10, 10));

	const std::string screen_path = fs::get_config_dir() + "screenshots/";
	const QStringList filter{ QStringLiteral("*.png") };
	QDirIterator dir_iter(QString::fromStdString(screen_path), filter, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

	QPixmap placeholder(m_icon_size);
	placeholder.fill(Qt::gray);
	m_placeholder = QIcon(placeholder);

	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		QListWidgetItem* item = new QListWidgetItem;
		item->setData(item_role::source, filepath);
		item->setData(item_role::loaded, false);
		item->setIcon(m_placeholder);
		item->setToolTip(filepath);

		m_grid->addItem(item);
	}

	m_icon_loader = new QFutureWatcher<thumbnail>(this);
	connect(m_icon_loader, &QFutureWatcher<QIcon>::resultReadyAt, this, &screenshot_manager_dialog::update_icon);

	connect(m_grid, &QListWidget::itemDoubleClicked, this, &screenshot_manager_dialog::show_preview);
	connect(m_grid->verticalScrollBar(), &QScrollBar::valueChanged, this, &screenshot_manager_dialog::update_icons);

	QVBoxLayout* layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_grid);
	setLayout(layout);

	resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
}

screenshot_manager_dialog::~screenshot_manager_dialog()
{
	m_icon_loader->cancel();
	m_icon_loader->waitForFinished();
}

void screenshot_manager_dialog::show_preview(QListWidgetItem* item)
{
	if (!item)
	{
		return;
	}

	const QString filepath = item->data(Qt::UserRole).toString();

	screenshot_preview* preview = new screenshot_preview(filepath);
	preview->show();
}

void screenshot_manager_dialog::update_icon(int index)
{
	const thumbnail tn = m_icon_loader->resultAt(index);

	if (QListWidgetItem* item = m_grid->item(tn.index))
	{
		item->setIcon(tn.icon);
		item->setData(item_role::loaded, true);
	}
}

void screenshot_manager_dialog::update_icons(int value)
{
	const QRect visible_rect = rect();

	QList<thumbnail> thumbnails_to_load;

	const bool forward = value >= m_scrollbar_value;
	m_scrollbar_value  = value;

	const int first = forward ? 0 : (m_grid->count() - 1);
	const int last  = forward ? (m_grid->count() - 1) : 0;

	for (int i = first; forward ? i <= last : i >= last; forward ? ++i : --i)
	{
		if (QListWidgetItem* item = m_grid->item(i))
		{
			const bool is_loaded  = item->data(item_role::loaded).toBool();
			const bool is_visible = visible_rect.intersects(m_grid->visualItemRect(item));

			if (is_visible)
			{
				if (!is_loaded)
				{
					thumbnails_to_load.push_back({ QIcon(), item->data(item_role::source).toString() , i });
				}
			}
			else if (is_loaded)
			{
				item->setIcon(m_placeholder);
				item->setData(item_role::loaded, false);
			}
		}
	}

	if (m_icon_loader->isRunning())
	{
		m_icon_loader->cancel();
		m_icon_loader->waitForFinished();
	}

	std::function<thumbnail(thumbnail)> load = [icon_size = m_icon_size](thumbnail tn) -> thumbnail
	{
		QPixmap pixmap(tn.path);
		tn.icon = QIcon(pixmap.scaled(icon_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		return tn;
	};

	m_icon_loader->setFuture(QtConcurrent::mapped(thumbnails_to_load, load));
}

void screenshot_manager_dialog::resizeEvent(QResizeEvent* event)
{
	QDialog::resizeEvent(event);
	update_icons(m_scrollbar_value);
}
