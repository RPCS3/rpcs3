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
	u32 m_start_addr = 0;
	u32 m_item_count = 30;
	u32 m_selected_instruction = -1;
	bool m_follow_thread = true; // If true, follow the selected thread to wherever it goes in code
	bool m_showing_selected_instruction = false;
	bool m_dirty_flag = false;
	QColor m_color_bp;
	QColor m_color_pc;
	QColor m_text_color_bp;
	QColor m_text_color_pc;

Q_SIGNALS:
	void BreakpointRequested(u32 loc, bool only_add = false);
public:
	debugger_list(QWidget* parent, std::shared_ptr<gui_settings> settings, breakpoint_handler* handler);
	void UpdateCPUData(std::shared_ptr<CPUDisAsm> disasm);
	void EnableThreadFollowing(bool enable = true);
public Q_SLOTS:
	void ShowAddress(u32 addr, bool select_addr = true, bool direct = false);
	void RefreshView();
protected:
	void keyPressEvent(QKeyEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void showEvent(QShowEvent* event) override;
	void hideEvent(QHideEvent* event) override;
	void scroll(s32 steps);
	void create_rsx_command_detail(u32 pc);
private:
	/**
	* It really upsetted me I had to copy this code to make debugger_list/frame not circularly dependent.
	*/
	u32 GetStartAddress(u32 address);
	bool IsSpu() const;

	std::shared_ptr<gui_settings> m_gui_settings;

	breakpoint_handler* m_ppu_breakpoint_handler = nullptr;
	cpu_thread* m_cpu = nullptr;
	std::shared_ptr<CPUDisAsm> m_disasm;
	QDialog* m_cmd_detail = nullptr;
	QLabel* m_detail_label = nullptr;
};
