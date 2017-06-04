#ifndef KERNELEXPLORER_H
#define KERNELEXPLORER_H

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTreeWidget>
#include <QHeaderView>

class kernel_explorer : public QDialog
{
	Q_OBJECT

	QTreeWidget* m_tree;

public:
	kernel_explorer(QWidget* parent);
private slots:
	void Update();
};

#endif // KERNELEXPLORER_H
