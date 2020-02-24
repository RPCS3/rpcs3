#pragma once

#include "stdafx.h"

#include <QListWidget>

class breakpoint_handler;
class CPUDisAsm;
class cpu_thread;
class gui_settings;

class debugger_list : public QListWidget
{
	Q_OBJECT

public:
	u32 m_pc = 0;
	u32 m_item_count = 30;
	bool m_no_thread_selected;
	QColor m_color_bp;
	QColor m_color_pc;
	QColor m_text_color_bp;
	QColor m_text_color_pc;

Q_SIGNALS:
	void BreakpointRequested(u32 loc);
public:
	debugger_list(QWidget* parent, std::shared_ptr<gui_settings> settings, breakpoint_handler* handler);
	void UpdateCPUData(std::weak_ptr<cpu_thread> cpu, std::shared_ptr<CPUDisAsm> disasm);
public Q_SLOTS:
	void ShowAddress(u32 addr);
protected:
	void keyPressEvent(QKeyEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
private:
	/**
	* It really upsetted me I had to copy this code to make debugger_list/frame not circularly dependent.
	*/
	u32 GetPc() const;
	u32 GetCenteredAddress(u32 address) const;

	std::shared_ptr<gui_settings> xgui_settings;

	breakpoint_handler* m_breakpoint_handler;
	std::weak_ptr<cpu_thread> cpu;
	std::shared_ptr<CPUDisAsm> m_disasm;
};
