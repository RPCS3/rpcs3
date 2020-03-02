#pragma once

#include <QDialog>
#include <QTreeWidget>

class kernel_explorer : public QDialog
{
	Q_OBJECT

	QTreeWidget* m_tree;

public:
	kernel_explorer(QWidget* parent);
private Q_SLOTS:
	void Update();
};
