#pragma once

#include <QListWidgetItem>

class numbered_widget_item final : public QListWidgetItem
{
public:
	explicit numbered_widget_item(const QString& text, QListWidget* listview = nullptr, int type = QListWidgetItem::Type)
		: QListWidgetItem(text, listview, type)
	{
	}

	QVariant data(int role) const override
	{
		switch (role)
		{
		case Qt::DisplayRole:
			// Return number and original display text (e.g. "14. My Cool Game (BLUS12345)")
			return QStringLiteral("%1. %2").arg(listWidget()->row(this) + 1).arg(QListWidgetItem::data(Qt::DisplayRole).toString());
		default:
			// Return original data
			return QListWidgetItem::data(role);
		}
	}

	bool operator<(const QListWidgetItem& other) const override
	{
		// Compare original display text
		return QListWidgetItem::data(Qt::DisplayRole).toString() < other.QListWidgetItem::data(Qt::DisplayRole).toString();
	}
};
