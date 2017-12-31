#pragma once

#include <QTableWidgetItem>

class custom_table_widget_item : public QTableWidgetItem
{
public:
	bool operator <(const QTableWidgetItem &other) const
	{
		return data(Qt::UserRole) < other.data(Qt::UserRole);
	}
};
