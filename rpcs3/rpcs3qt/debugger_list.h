#pragma once

#include "util/types.hpp"

#include <QListWidget>

#include <memory>

class breakpoint_handler;
class CPUDisAsm;
class cpu_thread;
class gui_settings;
class QLabel;

class debugger_list : public QListWidget
{
	Q_OBJECT

public:
	u32 m_pc = 0;
	u32 m_item_count = 30;
	QColor m_color_bp;
	QColor m_color_pc;
	QColor m_text_color_bp;
	QColor m_text_color_pc;

Q_SIGNALS:
	void BreakpointRequested(u32 loc);
public:
	debugger_list(QWidget* parent, std::shared_ptr<gui_settings> settings, breakpoint_handler* handler);
	void UpdateCPUData(cpu_thread* cpu, CPUDisAsm* disasm);
public Q_SLOTS:
	void ShowAddress(u32 addr, bool select_addr = true, bool force = false);
protected:
	void keyPressEvent(QKeyEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void showEvent(QShowEvent* event) override;
	void hideEvent(QHideEvent* event) override;
	void scroll(s32 steps);
	void create_rsx_command_detail(u32 pc, int row);
private:
	/**
	* It really upsetted me I had to copy this code to make debugger_list/frame not circularly dependent.
	*/
	u32 GetCenteredAddress(u32 address) const;

	std::shared_ptr<gui_settings> m_gui_settings;

	breakpoint_handler* m_breakpoint_handler;
	cpu_thread* m_cpu = nullptr;
	CPUDisAsm* m_disasm;
	QDialog* m_cmd_detail = nullptr;
	QLabel* m_detail_label = nullptr;
};
