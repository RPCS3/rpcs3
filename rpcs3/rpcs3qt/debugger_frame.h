#pragma once

#include "util/types.hpp"
#include "util/shared_ptr.hpp"

#include "custom_dock_widget.h"

#include <QSplitter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QComboBox>

#include <memory>
#include <vector>
#include <any>
#include <functional>

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

namespace utils
{
	class shm;
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

	debugger_list* m_debugger_list = nullptr;
	QSplitter* m_right_splitter = nullptr;
	QFont m_mono;
	QPlainTextEdit* m_misc_state = nullptr;
	QPlainTextEdit* m_regs = nullptr;
	QPushButton* m_go_to_addr = nullptr;
	QPushButton* m_go_to_pc = nullptr;
	QPushButton* m_btn_step = nullptr;
	QPushButton* m_btn_step_over = nullptr;
	QPushButton* m_btn_add_bp = nullptr;
	QPushButton* m_btn_run = nullptr;

	QComboBox* m_choice_units = nullptr;
	QTimer* m_update = nullptr;
	QSplitter* m_splitter = nullptr;

	u64 m_threads_created = -1;
	u64 m_threads_deleted = -1;
	system_state m_emu_state{};
	u64 m_emulation_id{};
	u32 m_last_pc = -1;
	std::vector<char> m_last_query_state;
	std::string m_last_reg_state;
	std::any m_dump_reg_func_data;
	std::vector<std::function<cpu_thread*()>> m_threads_info;
	u32 m_last_step_over_breakpoint = -1;
	u64 m_ui_update_ctr = 0;
	u64 m_ui_fast_update_permission_deadline = 0;
	bool m_thread_list_pending_update = false;

	std::shared_ptr<CPUDisAsm> m_disasm; // Only shared to allow base/derived functionality
	shared_ptr<cpu_thread> m_cpu;
	rsx::thread* m_rsx = nullptr;
	std::shared_ptr<utils::shm> m_spu_disasm_memory;
	u32 m_spu_disasm_origin_eal = 0;
	u32 m_spu_disasm_pc = 0;
	bool m_is_spu_disasm_mode = false;

	breakpoint_list* m_breakpoint_list = nullptr;
	breakpoint_handler* m_ppu_breakpoint_handler = nullptr;
	call_stack_list* m_call_stack_list = nullptr;
	instruction_editor_dialog* m_inst_editor = nullptr;
	register_editor_dialog* m_reg_editor = nullptr;
	QDialog* m_goto_dialog = nullptr;
	QDialog* m_spu_disasm_dialog = nullptr;

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
	void OnSelectSPUDisassembler();
	void OnRegsContextMenu(const QPoint& pos);
	void ShowPC(bool user_requested = false);
	void EnableUpdateTimer(bool enable) const;
	void RunBtnPress();
	void RegsShowMemoryViewerAction();
};

Q_DECLARE_METATYPE(u32)
