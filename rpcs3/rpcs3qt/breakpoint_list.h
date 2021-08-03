#pragma once

#include "util/types.hpp"

#include <QListWidget>

class CPUDisAsm;
class cpu_thread;
class breakpoint_handler;

class breakpoint_list : public QListWidget
{
	Q_OBJECT

public:
	breakpoint_list(QWidget* parent, breakpoint_handler* handler);
	void UpdateCPUData(cpu_thread* cpu, CPUDisAsm* disasm);
	void ClearBreakpoints();
	bool AddBreakpoint(u32 pc);
	void RemoveBreakpoint(u32 addr);

	QColor m_text_color_bp;
	QColor m_color_bp;
Q_SIGNALS:
	void RequestShowAddress(u32 addr, bool select_addr = true, bool force = false);
public Q_SLOTS:
	void HandleBreakpointRequest(u32 loc);
private Q_SLOTS:
	void OnBreakpointListDoubleClicked();
	void OnBreakpointListRightClicked(const QPoint &pos);
	void OnBreakpointListDelete();
private:
	breakpoint_handler* m_breakpoint_handler;
	QMenu* m_context_menu = nullptr;
	QAction* m_delete_action;
	cpu_thread* m_cpu = nullptr;
	CPUDisAsm* m_disasm = nullptr;
};
