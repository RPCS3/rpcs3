#pragma once

#include <rpcs3/rpcs3qt/trophy_manager_dialog.h>

#include <QTreeWidgetItem>


class trophy_tree_widget_item : public QTreeWidgetItem
{
public:
	trophy_tree_widget_item(QTreeWidget* parent) : QTreeWidgetItem(parent) {};
	trophy_tree_widget_item(QTreeWidgetItem* parent) : QTreeWidgetItem(parent) {};
private:
	bool operator<(const QTreeWidgetItem &other) const {
		auto GetTrophyRank = [](const QString& name)
		{
			if (name.toLower() == "bronze")
			{
				return 0;
			}
			else if (name.toLower() == "silver")
			{
				return 1;
			}
			else if (name.toLower() == "gold")
			{
				return 2;
			}
			else if (name.toLower() == "platinum")
			{
				return 3;
			}
			return -1;
		};

		int column = treeWidget()->sortColumn();
		switch (column)
		{
		case TrophyColumns::Name: return text(TrophyColumns::Name).toLower() < other.text(TrophyColumns::Name).toLower();
		case TrophyColumns::Description: return text(TrophyColumns::Description).toLower() < other.text(TrophyColumns::Description).toLower();
		case TrophyColumns::Type: return GetTrophyRank(text(TrophyColumns::Type)) < GetTrophyRank(other.text(TrophyColumns::Type));
		case TrophyColumns::IsUnlocked: return text(TrophyColumns::IsUnlocked).toLower() > other.text(TrophyColumns::IsUnlocked).toLower();
		case TrophyColumns::Id:
		default:
			return text(TrophyColumns::Id).toInt() < other.text(TrophyColumns::Id).toInt();
		}
	}
};
