#pragma once

#include "util/types.hpp"

#include <QListWidget>

#include "breakpoint_handler.h"

class CPUDisAsm;
class cpu_thread;

class breakpoint_list : public QListWidget
{
	Q_OBJECT

public:
	breakpoint_list(QWidget* parent, breakpoint_handler* handler);
	void UpdateCPUData(std::shared_ptr<CPUDisAsm> disasm);
	void ClearBreakpoints();
	void RemoveBreakpoint(u32 addr);
	bool AddBreakpoint(u32 pc, bs_t<breakpoint_types> type);

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
	breakpoint_handler* m_ppu_breakpoint_handler = nullptr;
	QMenu* m_context_menu = nullptr;
	QAction* m_delete_action;
	std::shared_ptr<CPUDisAsm> m_disasm = nullptr;
};
