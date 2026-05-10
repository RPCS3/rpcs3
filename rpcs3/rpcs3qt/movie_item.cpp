#include "stdafx.h"
#include "movie_item.h"

movie_item::movie_item() : QTableWidgetItem(), movie_item_base()
{
}

movie_item::movie_item(const QString& text, int type) : QTableWidgetItem(text, type), movie_item_base()
{
}

movie_item::movie_item(const QIcon& icon, const QString& text, int type) : QTableWidgetItem(icon, text, type), movie_item_base()
{
}
