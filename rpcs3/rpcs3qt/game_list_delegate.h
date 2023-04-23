#pragma once

#include "table_item_delegate.h"

class game_list_delegate : public table_item_delegate
{
public:
	explicit game_list_delegate(QObject* parent);

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
