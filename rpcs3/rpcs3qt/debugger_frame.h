#pragma once

#include "util/types.hpp"

#include "custom_dock_widget.h"

#include <QSplitter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QComboBox>

#include <memory>
#include <vector>
#include <any>

class CPUDisAsm;
class cpu_thread;
class gui_settings;
class debugger_list;
class breakpoint_list;
class breakpoint_handler;
class call_stack_list;

namespace rsx
{
	class thread;
}

enum class system_state : u32;

class instruction_editor_dialog;
class register_editor_dialog;

class debugger_frame : public custom_dock_widget
{
	Q_OBJECT

	const QString NoThreadString = tr("No Thread");
	const QString RunString = tr("Run");
	const QString PauseString = tr("Pause");

	debugger_list* m_debugger_list;
	QSplitter* m_right_splitter;
	QFont m_mono;
	QPlainTextEdit* m_misc_state;
	QPlainTextEdit* m_regs;
	QPushButton* m_go_to_addr;
	QPushButton* m_go_to_pc;
	QPushButton* m_btn_step;
	QPushButton* m_btn_step_over;
	QPushButton* m_btn_run;
	QComboBox* m_choice_units;
	QTimer* m_update;
	QSplitter* m_splitter;

	u64 m_threads_created = -1;
	u64 m_threads_deleted = -1;
	system_state m_emu_state{};
	u32 m_last_pc = -1;
	std::vector<char> m_last_query_state;
	std::string m_last_reg_state;
	std::any m_dump_reg_func_data;
	u32 m_last_step_over_breakpoint = -1;
	u64 m_ui_update_ctr = 0;
	u64 m_ui_fast_update_permission_deadline = 0;

	std::shared_ptr<CPUDisAsm> m_disasm; // Only shared to allow base/derived functionality
	std::shared_ptr<cpu_thread> m_cpu;
	rsx::thread* m_rsx = nullptr;

	breakpoint_list* m_breakpoint_list;
	breakpoint_handler* m_ppu_breakpoint_handler;
	call_stack_list* m_call_stack_list;
	instruction_editor_dialog* m_inst_editor = nullptr;
	register_editor_dialog* m_reg_editor = nullptr;
	QDialog* m_goto_dialog = nullptr;

	std::shared_ptr<gui_settings> m_gui_settings;

	cpu_thread* get_cpu();
	std::function<cpu_thread*()> make_check_cpu(cpu_thread* cpu, bool unlocked = false);
	void open_breakpoints_settings();

public:
	explicit debugger_frame(std::shared_ptr<gui_settings> settings, QWidget *parent = nullptr);

	void SaveSettings() const;
	void ChangeColors() const;

	void UpdateUI();
	void UpdateUnitList();

	void DoUpdate();
	void WritePanels();
	void EnableButtons(bool enable);
	void ShowGotoAddressDialog();
	void PerformGoToRequest(const QString& text_argument);
	void PerformGoToThreadRequest(const QString& text_argument);
	void PerformAddBreakpointRequest(u32 addr);
	u64 EvaluateExpression(const QString& expression);
	void ClearBreakpoints() const; // Fallthrough method into breakpoint_list.
	void ClearCallStack();

	/** Needed so key press events work when other objects are selected in debugger_frame. */
	bool eventFilter(QObject* object, QEvent* event) override;
protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event) override;
	void showEvent(QShowEvent* event) override;
	void hideEvent(QHideEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;

Q_SIGNALS:
	void DebugFrameClosed();
	void CallStackUpdateRequested(const std::vector<std::pair<u32, u32>>& call_stack);

public Q_SLOTS:
	void DoStep(bool step_over = false);

private Q_SLOTS:
	void OnSelectUnit();
	void ShowPC(bool user_requested = false);
	void EnableUpdateTimer(bool enable) const;
	void RunBtnPress();
};

Q_DECLARE_METATYPE(u32)
