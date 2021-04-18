#include "game_list_grid.h"
#include "game_list_grid_delegate.h"
#include "movie_item.h"
#include "qt_utils.h"

#include <QHeaderView>
#include <QScrollBar>

game_list_grid::game_list_grid(const QSize& icon_size, QColor icon_color, const qreal& margin_factor, const qreal& text_factor, const bool& showText)
	: game_list()
	, m_icon_size(icon_size)
	, m_icon_color(std::move(icon_color))
	, m_margin_factor(margin_factor)
	, m_text_factor(text_factor)
	, m_text_enabled(showText)
{
	setObjectName("game_grid");

	QSize item_size;
	if (m_text_enabled)
	{
		item_size = m_icon_size + QSize(m_icon_size.width() * m_margin_factor * 2, m_icon_size.height() * m_margin_factor * (m_text_factor + 1));
	}
	else
	{
		item_size = m_icon_size + m_icon_size * m_margin_factor * 2;
	}

	grid_item_delegate = new game_list_grid_delegate(item_size, m_margin_factor, m_text_factor, this);
	setItemDelegate(grid_item_delegate);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setSelectionBehavior(QAbstractItemView::SelectItems);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	verticalScrollBar()->setSingleStep(20);
	horizontalScrollBar()->setSingleStep(20);
	setContextMenuPolicy(Qt::CustomContextMenu);
	verticalHeader()->setVisible(false);
	horizontalHeader()->setVisible(false);
	setShowGrid(false);
	setMouseTracking(true);
}

void game_list_grid::enableText(const bool& enabled)
{
	m_text_enabled = enabled;
}

void game_list_grid::setIconSize(const QSize& size) const
{
	if (m_text_enabled)
	{
		grid_item_delegate->setItemSize(size + QSize(size.width() * m_margin_factor * 2, size.height() * m_margin_factor * (m_text_factor + 1)));
	}
	else
	{
		grid_item_delegate->setItemSize(size + size * m_margin_factor * 2);
	}
}

movie_item* game_list_grid::addItem(const game_info& app, const QString& name, const QString& movie_path, const int& row, const int& col)
{
	// create item with expanded image, title and position
	movie_item* item = new movie_item;

	item->set_icon_func([this, app, item](int)
	{
		ensure(item);

		const qreal device_pixel_ratio = devicePixelRatioF();

		// define size of expanded image, which is raw image size + margins
		QSizeF exp_size_f;
		if (m_text_enabled)
		{
			exp_size_f = m_icon_size + QSizeF(m_icon_size.width() * m_margin_factor * 2, m_icon_size.height() * m_margin_factor * (m_text_factor + 1));
		}
		else
		{
			exp_size_f = m_icon_size + m_icon_size * m_margin_factor * 2;
		}

		QMovie* movie = item->movie();
		const bool draw_movie_frame = movie && movie->isValid() && item->get_active();
		const QSize exp_size = (exp_size_f * device_pixel_ratio).toSize();

		// create empty canvas for expanded image
		QImage exp_img(exp_size, QImage::Format_ARGB32);
		exp_img.setDevicePixelRatio(device_pixel_ratio);
		exp_img.fill(Qt::transparent);

		// define offset for raw image placement
		QPoint offset(m_icon_size.width() * m_margin_factor, m_icon_size.height() * m_margin_factor);

		// place raw image inside expanded image
		QPainter painter(&exp_img);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);

		if (draw_movie_frame)
		{
			const QPixmap scaled_movie_frame = movie->currentPixmap().scaled(m_icon_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			offset += QPoint(m_icon_size.width() / 2 - scaled_movie_frame.width() / 2,
			                 m_icon_size.height() / 2 - scaled_movie_frame.height() / 2);
			painter.drawPixmap(offset, scaled_movie_frame);
		}
		else
		{
			// create background for image
			QImage bg_img(app->pxmap.size(), QImage::Format_ARGB32);
			bg_img.setDevicePixelRatio(device_pixel_ratio);
			bg_img.fill(m_icon_color);

			painter.drawImage(offset, bg_img);
			painter.drawPixmap(offset, app->pxmap);

			if (movie)
			{
				movie->stop();
			}
		}

		painter.end();

		// create item with expanded image, title and position
		item->setData(Qt::ItemDataRole::DecorationRole, QPixmap::fromImage(exp_img));
	});

	if (!movie_path.isEmpty())
	{
		item->init_movie(movie_path);
	}

	if (m_text_enabled)
	{
		item->setData(Qt::ItemDataRole::DisplayRole, name);
	}

	setItem(row, col, item);
	return item;
}

qreal game_list_grid::getMarginFactor() const
{
	return m_margin_factor;
}
