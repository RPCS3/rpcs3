#include "stdafx.h"
#include "screenshot_item.h"
#include "qt_utils.h"
#include "Utilities/Thread.h"
#include "Utilities/File.h"

#include <QVBoxLayout>
#include <QMenu>
#include <QGuiApplication>
#include <QClipboard>
#include <QImage>
#include <QDir>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>

LOG_CHANNEL(gui_log, "GUI");

screenshot_item::screenshot_item(QWidget* parent, QSize icon_size, const QString& icon_path, const QPixmap& placeholder)
	: flow_widget_item(parent)
	, m_icon_path(icon_path)
	, m_icon_size(icon_size)
{
	setObjectName("screenshot_item");
	setFocusPolicy(Qt::StrongFocus);
	setToolTip(icon_path);

	cb_on_first_visibility = [this]()
	{
		m_thread.reset(QThread::create([this]()
		{
			thread_base::set_name("Screenshot item");

			const QPixmap src_icon(m_icon_path);
			if (src_icon.isNull())
			{
				return;
			}

			const QPixmap pixmap = gui::utils::get_aligned_pixmap(src_icon, m_icon_size, 1.0, Qt::SmoothTransformation, gui::utils::align_h::center, gui::utils::align_v::center);
			Q_EMIT signal_icon_update(pixmap);
		}));
		m_thread->start();
	};

	m_label = new QLabel(this);
	m_label->setObjectName("screenshot_item_label");
	m_label->setAttribute(Qt::WA_TransparentForMouseEvents);
	m_label->setPixmap(placeholder);
	m_label->setFixedSize(m_icon_size);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(3, 3, 3, 3);
	layout->setAlignment(Qt::AlignCenter);
	layout->addWidget(m_label);

	setFixedSize(m_icon_size + QSize(6, 6));

	connect(this, &screenshot_item::signal_icon_update, this, &screenshot_item::update_icon, Qt::ConnectionType::QueuedConnection);
}

screenshot_item::~screenshot_item()
{
	if (m_thread && m_thread->isRunning())
	{
		m_thread->wait();
	}
}

void screenshot_item::polish_style()
{
	flow_widget_item::polish_style();
	update();
}

void screenshot_item::paintEvent(QPaintEvent* event)
{
	flow_widget_item::paintEvent(event);

	if (selected || hasFocus())
	{
		QPainter painter(this);
		painter.fillRect(rect(), palette().color(QPalette::Highlight));
	}
	else if (m_hover)
	{
		QPainter painter(this);
		QColor hover_color = palette().color(QPalette::Highlight);
		hover_color.setAlpha(80);
		painter.fillRect(rect(), hover_color);
	}
}

void screenshot_item::update_icon(const QPixmap& pixmap)
{
	if (m_label)
	{
		m_label->setPixmap(pixmap);
	}
}

void screenshot_item::mousePressEvent(QMouseEvent* ev)
{
	flow_widget_item::mousePressEvent(ev);

	if (!ev)
	{
		return;
	}

	if (ev->button() == Qt::LeftButton)
	{
		setFocus();
	}
}

void screenshot_item::mouseDoubleClickEvent(QMouseEvent* ev)
{
	flow_widget_item::mouseDoubleClickEvent(ev);

	if (!ev)
	{
		return;
	}

	if (ev->button() == Qt::LeftButton)
	{
		Q_EMIT signal_icon_preview(m_icon_path);
	}
}

void screenshot_item::contextMenuEvent(QContextMenuEvent* ev)
{
	if (!ev)
	{
		return;
	}

	setFocus();
	show_context_menu(ev->globalPos());
}

void screenshot_item::keyPressEvent(QKeyEvent* ev)
{
	if (ev && ev->key() == Qt::Key_Delete)
	{
		delete_screenshot();
		return;
	}

	flow_widget_item::keyPressEvent(ev);
}

void screenshot_item::show_context_menu(const QPoint& global_pos)
{
	QMenu menu(this);

	menu.addAction(tr("&Open Preview"), this, [this]()
	{
		Q_EMIT signal_icon_preview(m_icon_path);
	});

	menu.addSeparator();

	menu.addAction(tr("&Open file location"), this, [this]()
	{
		gui::utils::open_dir(m_icon_path);
	});

	menu.addAction(tr("&Copy"), this, [this]()
	{
		const QImage image(m_icon_path);
		if (!image.isNull())
		{
			QGuiApplication::clipboard()->setImage(image);
		}
	});

	menu.addAction(tr("Copy &Path"), this, [this]()
	{
		QGuiApplication::clipboard()->setText(QDir::toNativeSeparators(m_icon_path));
	});

	menu.addSeparator();

	menu.addAction(tr("&Delete"), this, &screenshot_item::delete_screenshot);

	menu.exec(global_pos);
}

void screenshot_item::delete_screenshot()
{
	if (m_icon_path.isEmpty())
	{
		return;
	}

	const auto answer = QMessageBox::question(this, tr("Delete Confirmation"),
		tr("Are you sure you want to delete this screenshot?\n%1").arg(QDir::toNativeSeparators(m_icon_path)));

	if (answer != QMessageBox::Yes)
	{
		return;
	}

	const std::string path_str = m_icon_path.toStdString();
	if (fs::is_file(path_str))
	{
		if (!fs::remove_file(path_str))
		{
			gui_log.error("Screenshot manager: Failed to remove screenshot '%s': %s", path_str, fs::g_tls_error);
			QMessageBox::warning(this, tr("Error"), tr("Failed to delete screenshot."));
			return;
		}

		gui_log.notice("Screenshot manager: Deleted screenshot '%s'", path_str);
	}

	Q_EMIT signal_screenshot_deleted();
}
