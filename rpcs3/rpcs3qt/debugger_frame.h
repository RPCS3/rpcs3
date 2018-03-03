#pragma once

#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/SPUDisAsm.h"
#include "Emu/Cell/PPUInterpreter.h"

#include "debugger_list.h"
#include "breakpoint_handler.h"
#include "instruction_editor_dialog.h"
#include "register_editor_dialog.h"
#include "gui_settings.h"

#include <QDockWidget>
#include <QListWidget>
#include <QPushButton>
#include <QComboBox>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QTextEdit>
#include <QSplitter>

class debugger_frame : public QDockWidget
{
	Q_OBJECT

	debugger_list* m_list;
	QSplitter* m_right_splitter;
	QFont m_mono;
	QTextEdit* m_regs;
	QPushButton* m_go_to_addr;
	QPushButton* m_go_to_pc;
	QPushButton* m_btn_capture;
	QPushButton* m_btn_step;
	QPushButton* m_btn_run;
	QComboBox* m_choice_units;
	QString m_current_choice;
	bool m_no_thread_selected = true;

	u64 m_threads_created = 0;
	u64 m_threads_deleted = 0;
	u32 m_last_pc = -1;
	u32 m_last_stat = 0;

	QTimer* m_update;
	QSplitter* m_splitter;

	const QString NoThread = tr("No Thread");
	const QString Run = tr("Run");
	const QString Pause = tr("Pause");

	std::shared_ptr<gui_settings> xgui_settings;

	breakpoint_handler* m_brkpt_handler; // Handles communicating with PPU to simplify logic in debugger.
	QAction* m_breakpoints_list_delete;
public:
	std::shared_ptr<CPUDisAsm> m_disasm;
	QListWidget* m_breakpoints_list;
	std::weak_ptr<cpu_thread> cpu;

public:
	explicit debugger_frame(std::shared_ptr<gui_settings> settings, QWidget *parent = 0);
	void SaveSettings();
	void ChangeColors();

	void UpdateUI();
	void UpdateUnitList();

	u32 GetPc() const;
	u32 CentrePc(u32 pc) const;
	void DoUpdate();
	void WriteRegs();
	void EnableButtons(bool enable);
	void ClearBreakpoints();
	void RemoveBreakpoint(u32 loc);
	void AddBreakpoint(u32 loc);

	void OnUpdate();

protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event);
	void showEvent(QShowEvent* event);
	void hideEvent(QHideEvent* event);

Q_SIGNALS:
	void DebugFrameClosed();
public Q_SLOTS:
	void DoStep();
private Q_SLOTS:
	void HandleBreakpointRequest(u32 loc);
	void OnBreakpointListDoubleClicked();
	void OnBreakpointListRightClicked(const QPoint &pos);
	void OnBreakpointListDelete();
	void OnSelectUnit();
	void Show_Val();
	void Show_PC();
	void EnableUpdateTimer(bool state);
};

Q_DECLARE_METATYPE(u32);