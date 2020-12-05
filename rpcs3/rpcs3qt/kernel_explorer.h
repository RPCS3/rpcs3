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
		memory_containers = sys_size,
		virtual_memory,
		sockets,
		ppu_threads,
		spu_threads,
		spu_thread_groups,
		rsx_contexts,
		file_descriptors,
		process_info,
	};

public:
	kernel_explorer(QWidget* parent);

private:
	QTreeWidget* m_tree;

private Q_SLOTS:
	void Update();
};
