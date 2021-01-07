#pragma once

#include <QTableWidgetItem>

class custom_table_widget_item : public QTableWidgetItem
{
private:
	int m_sort_role = Qt::DisplayRole;

public:
	using QTableWidgetItem::setData;

	custom_table_widget_item(){}
	custom_table_widget_item(const std::string& text, int sort_role = Qt::DisplayRole, const QVariant& sort_value = 0)
	: QTableWidgetItem(QString::fromStdString(text).simplified()) // simplified() forces single line text
	{
		if (sort_role != Qt::DisplayRole)
		{
			setData(sort_role, sort_value, true);
		}
	}
	custom_table_widget_item(const QString& text, int sort_role = Qt::DisplayRole, const QVariant& sort_value = 0)
	: QTableWidgetItem(text.simplified()) // simplified() forces single line text
	{
		if (sort_role != Qt::DisplayRole)
		{
			setData(sort_role, sort_value, true);
		}
	}

	bool operator <(const QTableWidgetItem& other) const
	{
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
		return data(m_sort_role) < other.data(m_sort_role);
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
	}

	void setData(int role, const QVariant& value, bool assign_sort_role)
	{
		if (assign_sort_role)
		{
			m_sort_role = role;
		}
		QTableWidgetItem::setData(role, value);
	}
};
