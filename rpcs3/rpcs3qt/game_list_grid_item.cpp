#include "game_list_grid_item.h"

#include <QVBoxLayout>
#include <QStyle>

game_list_grid_item::game_list_grid_item(QWidget* parent, game_info game, const QString& title)
	: flow_widget_item(parent), movie_item_base(), m_game(std::move(game))
{
	setObjectName("game_list_grid_item");
	setAttribute(Qt::WA_Hover); // We need to enable the hover attribute to ensure that hover events are handled.

	cb_on_first_visibility = [this]()
	{
		if (!icon_loading())
		{
			call_icon_load_func(0);
		}
	};

	m_icon_label = new QLabel(this);
	m_icon_label->setObjectName("game_list_grid_item_icon_label");
	m_icon_label->setAttribute(Qt::WA_TranslucentBackground);
	m_icon_label->setScaledContents(true);

	m_title_label = new QLabel(title, this);
	m_title_label->setObjectName("game_list_grid_item_title_label");
	m_title_label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	m_title_label->setWordWrap(true);
	m_title_label->setVisible(false);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_icon_label, 1);
	layout->addWidget(m_title_label, 0);

	setLayout(layout);
}

void game_list_grid_item::set_icon_size(const QSize& size)
{
	m_icon_size = size;
}

void game_list_grid_item::set_icon(const QPixmap& pixmap)
{
	m_icon_size = pixmap.size() / devicePixelRatioF();
	m_icon_label->setPixmap(pixmap);
}

void game_list_grid_item::adjust_size()
{
	m_icon_label->setMinimumSize(m_icon_size);
	m_icon_label->setMaximumSize(m_icon_size);
	m_title_label->setMaximumWidth(m_icon_size.width());
}

void game_list_grid_item::show_title(bool visible)
{
	if (m_title_label)
	{
		m_title_label->setVisible(visible);
	}
}

void game_list_grid_item::polish_style()
{
	flow_widget_item::polish_style();

	m_title_label->style()->unpolish(m_title_label);
	m_title_label->style()->polish(m_title_label);
}

bool game_list_grid_item::event(QEvent* event)
{
	switch (event->type())
	{
	case QEvent::HoverEnter:
		set_active(true);
		break;
	case QEvent::HoverLeave:
		set_active(false);
		break;
	default:
		break;
	}

	return flow_widget_item::event(event);
}
