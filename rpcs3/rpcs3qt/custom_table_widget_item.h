#pragma once

#include <QTableWidgetItem>

class custom_table_widget_item : public QTableWidgetItem
{
private:
	int m_sort_role = Qt::DisplayRole;

public:
	custom_table_widget_item(){}
	custom_table_widget_item(const std::string& text, int sort_role = Qt::DisplayRole, int sort_index = 0)
	: QTableWidgetItem(qstr(text).simplified()) // simplified() forces single line text
	{
		if (sort_role != Qt::DisplayRole)
		{
			setData(sort_role, sort_index, true);
		}
	}

	bool operator <(const QTableWidgetItem &other) const
	{
		return data(m_sort_role) < other.data(m_sort_role);
	}

	void setData(int role, const QVariant &value, bool assign_sort_role = false)
	{
		if (assign_sort_role)
		{
			m_sort_role = role;
		}
		QTableWidgetItem::setData(role, value);
	}
};
