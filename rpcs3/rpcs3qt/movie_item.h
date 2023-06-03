#pragma once

#include "movie_item_base.h"

#include <QTableWidgetItem>

class movie_item : public QTableWidgetItem, public movie_item_base
{
public:
	movie_item();
	movie_item(const QString& text, int type = Type);
	movie_item(const QIcon& icon, const QString& text, int type = Type);
};
