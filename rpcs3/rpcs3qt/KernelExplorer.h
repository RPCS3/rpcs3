#ifndef KERNELEXPLORER_H
#define KERNELEXPLORER_H

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTreeWidget>
#include <QHeaderview>

class KernelExplorer : public QDialog
{
	Q_OBJECT

	QTreeWidget* m_tree;

public:
	KernelExplorer(QWidget* parent);
private slots:
	void Update();
};

#endif // KERNELEXPLORER_H
