#pragma once

#include "breakpoint_handler.h"

#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"

#include <QListWidget>

class debugger_list : public QListWidget
{
	Q_OBJECT

public:
	u32 m_pc;
	u32 m_item_count;
	bool m_no_thread_selected;
	QColor m_color_bp;
	QColor m_color_pc;
	QColor m_text_color_bp;
	QColor m_text_color_pc;

Q_SIGNALS:
	void BreakpointRequested(u32 loc);
	void RequestCPUStep();
public:
	debugger_list(QWidget* parent, breakpoint_handler* handler);
	void ShowAddr(u32 addr);
	void UpdateCPUData(std::weak_ptr<cpu_thread> cpu, std::shared_ptr<CPUDisAsm> disasm);
protected:
	void keyPressEvent(QKeyEvent* event);
	void mouseDoubleClickEvent(QMouseEvent* event);
	void wheelEvent(QWheelEvent* event);
	void resizeEvent(QResizeEvent* event);
private:
	/**
	* It really upsetted me I had to copy this code to make debugger_list/frame not circularly dependent.
	*/
	u32 GetPc() const;

	breakpoint_handler* m_brkpoint_handler;
	std::weak_ptr<cpu_thread> cpu;
	std::shared_ptr<CPUDisAsm> m_disasm;
};
