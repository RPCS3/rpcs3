#include "screenshot_manager_dialog.h"
#include "screenshot_preview.h"
#include "qt_utils.h"
#include "Utilities/File.h"

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QListWidget>
#include <QScreen>
#include <QVBoxLayout>

screenshot_manager_dialog::screenshot_manager_dialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle(tr("Screenshots"));

	m_grid = new QListWidget(this);
	m_grid->setViewMode(QListWidget::IconMode);
	m_grid->setMovement(QListWidget::Static);
	m_grid->setResizeMode(QListWidget::Adjust);
	m_grid->setIconSize(QSize(160, 90));
	m_grid->setGridSize(m_grid->iconSize() + QSize(10, 10));

	const std::string screen_path = fs::get_config_dir() + "/screenshots/";
	const QStringList filter{ QStringLiteral("*.png") };
	QDirIterator dir_iter(QString::fromStdString(screen_path), filter, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		QListWidgetItem* item = new QListWidgetItem;
		item->setData(Qt::UserRole, filepath);
		item->setIcon(QIcon(filepath));
		item->setToolTip(filepath);

		m_grid->addItem(item);
	}

	connect(m_grid, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item)
	{
		if (!item)
		{
			return;
		}

		const QString filepath = item->data(Qt::UserRole).toString();

		screenshot_preview* preview = new screenshot_preview(filepath);
		preview->show();
	});

	QVBoxLayout* layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_grid);
	setLayout(layout);

	resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
}
