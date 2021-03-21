#pragma once

#include "util/types.hpp"

#include "custom_dock_widget.h"

#include <QSplitter>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>

#include <memory>
#include <vector>

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
	QTextEdit* m_misc_state;
	QTextEdit* m_regs;
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
	u32 m_last_step_over_breakpoint = -1;

	std::shared_ptr<CPUDisAsm> m_disasm; // Only shared to allow base/derived functionality
	std::shared_ptr<cpu_thread> m_cpu;
	rsx::thread* m_rsx = nullptr;

	breakpoint_list* m_breakpoint_list;
	breakpoint_handler* m_breakpoint_handler;
	call_stack_list* m_call_stack_list;
	instruction_editor_dialog* m_inst_editor = nullptr;
	register_editor_dialog* m_reg_editor = nullptr;

	std::shared_ptr<gui_settings> xgui_settings;

	cpu_thread* get_cpu();
	std::function<cpu_thread*()> make_check_cpu(cpu_thread* cpu);

public:
	explicit debugger_frame(std::shared_ptr<gui_settings> settings, QWidget *parent = 0);

	void SaveSettings();
	void ChangeColors();

	void UpdateUI();
	void UpdateUnitList();

	void DoUpdate();
	void WritePanels();
	void EnableButtons(bool enable);
	void ShowGotoAddressDialog();
	u64 EvaluateExpression(const QString& expression);
	void ClearBreakpoints(); // Fallthrough method into breakpoint_list.
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
	void CallStackUpdateRequested(std::vector<std::pair<u32, u32>> call_stack);

public Q_SLOTS:
	void DoStep(bool step_over = false);

private Q_SLOTS:
	void OnSelectUnit();
	void ShowPC();
	void EnableUpdateTimer(bool state);
};

Q_DECLARE_METATYPE(u32)
