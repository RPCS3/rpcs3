#include "game_list_grid_delegate.h"

game_list_grid_delegate::game_list_grid_delegate(const QSize& size, const qreal& margin_factor, const qreal& text_factor, const QFont& font, const QColor& font_color, QObject *parent)
	 : QAbstractItemDelegate(parent), m_size(size), m_margin_factor(margin_factor), m_text_factor(text_factor), m_font(font), m_font_color(font_color)
{
}

game_list_grid_delegate::~game_list_grid_delegate()
{
}

void game_list_grid_delegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QRect r = option.rect;

	painter->eraseRect(r);

	//Get title and image
	QPixmap image = (qvariant_cast<QPixmap>(index.data(Qt::DecorationRole)));
	QString title = index.data(Qt::DisplayRole).toString();

	// image
	if (image.isNull() == false)
	{
		painter->drawPixmap(r, image);
	}

	// Add selection overlay
	if (option.state & QStyle::State_Selected)
	{
		QLinearGradient gradientSelected(r.left(), r.top(), r.left(), r.height() + r.top());
		gradientSelected.setColorAt(0.0, QColor::fromRgba(qRgba(119, 213, 247, 128)));
		gradientSelected.setColorAt(0.9, QColor::fromRgba(qRgba(27, 134, 183, 128)));
		gradientSelected.setColorAt(1.0, QColor::fromRgba(qRgba(0, 120, 174, 128)));
		painter->fillRect(r, gradientSelected);
	}

	int h = r.height() / (1 + m_margin_factor + m_margin_factor*m_text_factor);
	int height = r.height() - h - h * m_margin_factor;
	int top = r.bottom() - height;

	// title
	painter->setPen(QPen(m_font_color, 1, Qt::SolidLine));
	painter->setFont(m_font);
	painter->drawText(QRect(r.left(), top, r.width(), height), Qt::TextWordWrap | Qt::AlignCenter, title);
}

QSize game_list_grid_delegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	Q_UNUSED(option);
	Q_UNUSED(index);
	return m_size;
}
