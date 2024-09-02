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
	void UpdateCPUData(std::shared_ptr<CPUDisAsm> disasm);
	void ClearBreakpoints();
	bool AddBreakpoint(u32 pc);
	void RemoveBreakpoint(u32 addr);

	QColor m_text_color_bp;
	QColor m_color_bp;

protected:
	void mouseDoubleClickEvent(QMouseEvent* ev) override;

Q_SIGNALS:
	void RequestShowAddress(u32 addr, bool select_addr = true, bool force = false);
public Q_SLOTS:
	void HandleBreakpointRequest(u32 loc, bool add_only);

private Q_SLOTS:
	void OnBreakpointListDoubleClicked();
	void OnBreakpointListRightClicked(const QPoint &pos);
	void OnBreakpointListDelete();

private:
	breakpoint_handler* m_ppu_breakpoint_handler;
	QMenu* m_context_menu = nullptr;
	QAction* m_delete_action;
	std::shared_ptr<CPUDisAsm> m_disasm = nullptr;
};
