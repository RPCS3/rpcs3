#pragma once

#include <QDialog>
#include <QTreeWidget>

#include "Utilities/types.h"

class kernel_explorer : public QDialog
{
	Q_OBJECT

	static const size_t sys_size = 256;

	enum additional_nodes
	{
		memory_containers = sys_size + 0,
		ppu_threads       = sys_size + 1,
		spu_threads       = sys_size + 2,
		spu_thread_groups = sys_size + 3,
		rsx_contexts      = sys_size + 4,
		file_descriptors  = sys_size + 5,
	};

public:
	kernel_explorer(QWidget* parent);

private:
	QTreeWidget* m_tree;

private Q_SLOTS:
	void Update();
};
