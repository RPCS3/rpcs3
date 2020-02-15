#pragma once

#include "stdafx.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/SPUDisAsm.h"
#include "Emu/Cell/PPUInterpreter.h"

#include "breakpoint_handler.h"
#include "custom_dock_widget.h"
#include "instruction_editor_dialog.h"
#include "register_editor_dialog.h"
#include "gui_settings.h"
#include "debugger_list.h"
#include "breakpoint_list.h"

#include <QSplitter>
#include <QTextEdit>

class debugger_frame : public custom_dock_widget
{
	Q_OBJECT

	const QString NoThreadString = tr("No Thread");
	const QString RunString = tr("Run");
	const QString PauseString = tr("Pause");

	debugger_list* m_debugger_list;
	QSplitter* m_right_splitter;
	QFont m_mono;
	QTextEdit* m_regs;
	QPushButton* m_go_to_addr;
	QPushButton* m_go_to_pc;
	QPushButton* m_btn_capture;
	QPushButton* m_btn_step;
	QPushButton* m_btn_step_over;
	QPushButton* m_btn_run;
	QComboBox* m_choice_units;
	QString m_current_choice;
	QTimer* m_update;
	QSplitter* m_splitter;

	u64 m_threads_created = 0;
	u64 m_threads_deleted = 0;
	u32 m_last_pc = -1;
	u32 m_last_stat = 0;
	u32 m_last_step_over_breakpoint = -1;
	bool m_no_thread_selected = true;

	std::shared_ptr<CPUDisAsm> m_disasm;
	std::weak_ptr<cpu_thread> cpu;

	breakpoint_list* m_breakpoint_list;
	breakpoint_handler* m_breakpoint_handler;

	std::shared_ptr<gui_settings> xgui_settings;

public:
	explicit debugger_frame(std::shared_ptr<gui_settings> settings, QWidget *parent = 0);

	void SaveSettings();
	void ChangeColors();

	void UpdateUI();
	void UpdateUnitList();

	u32 GetPc() const;
	void DoUpdate();
	void WriteRegs();
	void EnableButtons(bool enable);
	void ShowGotoAddressDialog();
	u64 EvaluateExpression(const QString& expression);
	void ClearBreakpoints(); // Fallthrough method into breakpoint_list.

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

public Q_SLOTS:
	void DoStep(bool stepOver = false);

private Q_SLOTS:
	void OnSelectUnit();
	void ShowPC();
	void EnableUpdateTimer(bool state);
};

Q_DECLARE_METATYPE(u32)
