#include "screenshot_preview.h"
#include "qt_utils.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QImage>
#include <QImageReader>
#include <QMenu>
#include <QResizeEvent>

screenshot_preview::screenshot_preview(const QString& filepath, QWidget* parent)
	: QLabel(parent)
	, m_filepath(filepath)
{
	QImageReader reader(filepath);
	reader.setAutoTransform(true);

	m_image = reader.read();

	setWindowTitle(tr("Screenshot Viewer"));
	setObjectName("screenshot_preview");
	setContextMenuPolicy(Qt::CustomContextMenu);
	setAttribute(Qt::WA_DeleteOnClose);
	setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	setPixmap(QPixmap::fromImage(m_image));
	setMinimumSize(160, 90);

	connect(this, &screenshot_preview::customContextMenuRequested, this, &screenshot_preview::show_context_menu);
}

void screenshot_preview::show_context_menu(const QPoint& pos)
{
	QMenu* menu = new QMenu();
	menu->addAction(tr("&Copy"), [this]() { QGuiApplication::clipboard()->setImage(m_image); });
	menu->addSeparator();
	menu->addAction(tr("&Open file location"), [this]() { gui::utils::open_dir(m_filepath); });
	menu->addSeparator();

	QAction* reset_act = menu->addAction(tr("To &Normal Size"), [this]() { scale(m_image.size()); });
	reset_act->setEnabled(pixmap(Qt::ReturnByValue).size() != m_image.size());

	QAction* stretch_act = menu->addAction(tr("&Stretch to size"), [this]() { m_stretch = !m_stretch; scale(size()); });
	stretch_act->setCheckable(true);
	stretch_act->setChecked(m_stretch);

	menu->addSeparator();
	menu->addAction(tr("E&xit"), this, &QLabel::close);
	menu->exec(mapToGlobal(pos));
}

void screenshot_preview::scale(const QSize& new_size)
{
	if (new_size != size())
	{
		resize(new_size);
	}

	setPixmap(QPixmap::fromImage(m_image.scaled(new_size, m_stretch ? Qt::IgnoreAspectRatio : Qt::KeepAspectRatio, Qt::SmoothTransformation)));
}

void screenshot_preview::resizeEvent(QResizeEvent* event)
{
	scale(event->size());
	event->ignore();
}
