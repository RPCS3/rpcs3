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

#include "custom_dock_widget.h"
#include "instruction_editor_dialog.h"
#include "register_editor_dialog.h"
#include "gui_settings.h"

#include <QListWidget>
#include <QPushButton>
#include <QComboBox>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QTextEdit>
#include <QSplitter>

class debugger_list;

class debugger_frame : public custom_dock_widget
{
	Q_OBJECT;

	const QString NoThreadString = tr("No Thread");
	const QString RunString = tr("Run");
	const QString PauseString = tr("Pause");

	debugger_list* m_list;
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
	QAction* m_breakpoints_list_delete;

	u64 m_threads_created = 0;
	u64 m_threads_deleted = 0;
	u32 m_last_pc = -1;
	u32 m_last_stat = 0;
	u32 m_last_step_over_breakpoint = -1;
	bool m_no_thread_selected = true;

	std::shared_ptr<gui_settings> xgui_settings;

public:
	std::unique_ptr<CPUDisAsm> m_disasm;
	QListWidget* m_breakpoints_list;
	std::weak_ptr<cpu_thread> cpu;

	explicit debugger_frame(std::shared_ptr<gui_settings> settings, QWidget *parent = 0);

	void SaveSettings();
	void ChangeColors();

	void UpdateUI();
	void UpdateUnitList();

	u32 GetPc() const;
	void DoUpdate();
	void WriteRegs();
	void EnableButtons(bool enable);
	void ClearBreakpoints();
	void ShowGotoAddressDialog();
	u64 EvaluateExpression(const QString& expression);

	void OnUpdate();

protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event) override;
	void showEvent(QShowEvent* event) override;
	void hideEvent(QHideEvent* event) override;

Q_SIGNALS:
	void DebugFrameClosed();

public Q_SLOTS:
	void DoStep(bool stepOver = false);

private Q_SLOTS:
	void OnBreakpointList_doubleClicked();
	void OnBreakpointList_rightClicked(const QPoint &pos);
	void OnBreakpointList_delete();
	void OnSelectUnit();
	void Show_PC();
	void EnableUpdateTimer(bool state);
};

class debugger_list : public QListWidget
{
	Q_OBJECT;

	debugger_frame* m_debugFrame;

public:
	u32 m_pc;
	u32 m_item_count;
	bool m_no_thread_selected;
	QColor m_color_bp;
	QColor m_color_pc;
	QColor m_text_color_bp;
	QColor m_text_color_pc;
	bool m_center_shown_addresses;

public:
	debugger_list(debugger_frame* parent, std::shared_ptr<gui_settings> settings);
	void ShowAddress(u32 addr);
	void RemoveBreakPoint(u32 pc, bool eraseFromMap = true);
	bool IsBreakPoint(u32 pc);
	void AddBreakPoint(u32 pc);

private:
	u32 GetCenteredAddress(u32 address);

protected:
	void keyPressEvent(QKeyEvent* event);
	void mouseDoubleClickEvent(QMouseEvent* event);
	void wheelEvent(QWheelEvent* event);
	void resizeEvent(QResizeEvent* event);
};

Q_DECLARE_METATYPE(u32);