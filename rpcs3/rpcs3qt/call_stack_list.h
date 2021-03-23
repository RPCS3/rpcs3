#pragma once

#include "util/types.hpp"

#include <QListWidget>
#include <vector>

class cpu_thread;
class CPUDisAsm;

class call_stack_list : public QListWidget
{
	Q_OBJECT

public:
	call_stack_list(QWidget* parent);

Q_SIGNALS:
	void RequestShowAddress(u32 addr, bool select_addr = true, bool force = false);
public Q_SLOTS:
	void HandleUpdate(std::vector<std::pair<u32, u32>> call_stack);
private Q_SLOTS:
	void OnCallStackListDoubleClicked();
};
