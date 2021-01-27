#pragma once

#include "util/types.hpp"

#include <QListWidget>
#include <memory>

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
	void AddBreakpoint(u32 addr);
	void RemoveBreakpoint(u32 addr);

	QColor m_text_color_bp;
	QColor m_color_bp;
Q_SIGNALS:
	void RequestShowAddress(u32 addr, bool force = false);
public Q_SLOTS:
	void HandleBreakpointRequest(u32 addr);
private Q_SLOTS:
	void OnBreakpointListDoubleClicked();
	void OnBreakpointListRightClicked(const QPoint &pos);
	void OnBreakpointListDelete();
private:
	breakpoint_handler* m_breakpoint_handler;

	cpu_thread* m_cpu;
	CPUDisAsm* m_disasm;
};
