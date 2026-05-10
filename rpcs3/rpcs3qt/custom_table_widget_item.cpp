#include "custom_table_widget_item.h"
#include "Utilities/StrFmt.h"

#include <QDateTime>

custom_table_widget_item::custom_table_widget_item(const std::string& text, int sort_role, const QVariant& sort_value)
    : movie_item(QString::fromStdString(text).simplified()) // simplified() forces single line text
{
	if (sort_role != Qt::DisplayRole)
	{
		setData(sort_role, sort_value, true);
	}
}

custom_table_widget_item::custom_table_widget_item(const QString& text, int sort_role, const QVariant& sort_value)
    : movie_item(text.simplified()) // simplified() forces single line text
{
	if (sort_role != Qt::DisplayRole)
	{
		setData(sort_role, sort_value, true);
	}
}

bool custom_table_widget_item::operator<(const QTableWidgetItem& other) const
{
	if (m_sort_role == Qt::DisplayRole)
	{
		return QTableWidgetItem::operator<(other);
	}

	const QVariant data_l = data(m_sort_role);
	const QVariant data_r = other.data(m_sort_role);
	const int type_l = data_l.metaType().id();
	const int type_r = data_r.metaType().id();

	ensure(type_l == type_r);

	switch (type_l)
	{
	case QMetaType::Type::Bool:
	case QMetaType::Type::Int:
		return data_l.toInt() < data_r.toInt();
	case QMetaType::Type::UInt:
		return data_l.toUInt() < data_r.toUInt();
	case QMetaType::Type::LongLong:
		return data_l.toLongLong() < data_r.toLongLong();
	case QMetaType::Type::ULongLong:
		return data_l.toULongLong() < data_r.toULongLong();
	case QMetaType::Type::Double:
		return data_l.toDouble() < data_r.toDouble();
	case QMetaType::Type::QDate:
		return data_l.toDate() < data_r.toDate();
	case QMetaType::Type::QTime:
		return data_l.toTime() < data_r.toTime();
	case QMetaType::Type::QDateTime:
		return data_l.toDateTime() < data_r.toDateTime();
	case QMetaType::Type::Char:
	case QMetaType::Type::QString:
		return data_l.toString() < data_r.toString();
	default:
		fmt::throw_exception("Unimplemented type %s", QMetaType(type_l).name());
	}
}

void custom_table_widget_item::setData(int role, const QVariant& value, bool assign_sort_role)
{
	if (assign_sort_role)
	{
		m_sort_role = role;
	}
	QTableWidgetItem::setData(role, value);
}
