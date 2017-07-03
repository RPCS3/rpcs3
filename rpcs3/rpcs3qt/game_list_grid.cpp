#include "game_list_grid.h"
#include "game_list_grid_delegate.h"

#include <QHeaderView>

game_list_grid::game_list_grid(const QSize& icon_size, const qreal& margin_factor, const qreal& text_factor, const bool& showText)
	: QTableWidget(), m_icon_size(icon_size), m_margin_factor(margin_factor), m_text_factor(text_factor), m_text_enabled(showText)
{
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
	setSelectionBehavior(QAbstractItemView::SelectItems);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	setContextMenuPolicy(Qt::CustomContextMenu);
	verticalHeader()->setVisible(false);
	horizontalHeader()->setVisible(false);
	setShowGrid(false);
}

game_list_grid::~game_list_grid()
{
}

void game_list_grid::enableText(const bool& enabled)
{
	m_text_enabled = enabled;
}

void game_list_grid::setIconSize(const QSize& size)
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

void game_list_grid::addItem(const QPixmap& img, const QString& name, const int& idx, const int& row, const int& col)
{
	// define size of expanded image, which is raw image size + margins
	QSize exp_size;
	if (m_text_enabled)
	{
		exp_size = m_icon_size + QSize(m_icon_size.width() * m_margin_factor * 2, m_icon_size.height() * m_margin_factor * (m_text_factor + 1));
	}
	else
	{
		exp_size = m_icon_size + m_icon_size * m_margin_factor * 2;
	}

	// define offset for raw image placement
	QPoint offset = QPoint(m_icon_size.width() * m_margin_factor, m_icon_size.height() * m_margin_factor);

	// create empty canvas for expanded image
	QImage exp_img = QImage(exp_size, QImage::Format_ARGB32);
	exp_img.fill(Qt::transparent);

	// create background for image
	QImage bg_img = QImage(img.size(), QImage::Format_ARGB32);
	bg_img.fill(QColor(209, 209, 209, 255));

	// place raw image inside expanded image
	QPainter painter(&exp_img);
	painter.drawImage(offset, bg_img);
	painter.drawPixmap(offset, img);
	painter.end();

	// create item with expanded image, title and position
	QTableWidgetItem* item = new QTableWidgetItem();
	item->setData(Qt::ItemDataRole::DecorationRole, QPixmap::fromImage(exp_img));
	item->setData(Qt::ItemDataRole::UserRole, idx);
	item->setData(Qt::ItemDataRole::ToolTipRole, name);
	if (m_text_enabled) { item->setData(Qt::ItemDataRole::DisplayRole, name); }
	setItem(row, col, item);
}

qreal game_list_grid::getMarginFactor()
{
	return m_margin_factor;
}
