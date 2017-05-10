#ifndef DEBUGGERFRAME_H
#define DEBUGGERFRAME_H

#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/PSP2/ARMv7Thread.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/SPUDisAsm.h"
#include "Emu/PSP2/ARMv7DisAsm.h"
#include "Emu/Cell/PPUInterpreter.h"

#include "InstructionEditor.h"
#include "RegisterEditor.h"

#include <QApplication>
#include <QDockWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QInputDialog>
#include <QFontDatabase>
#include <QTimer>
#include <QTextEdit>

class DebuggerFrame : public QDockWidget
{
	Q_OBJECT

	QWidget* body;
	QListWidget* m_list;
	std::unique_ptr<CPUDisAsm> m_disasm;
	u32 m_pc;
	int pSize;
	QFont mono;
	QTextEdit* m_regs;
	QPushButton* m_btn_step;
	QPushButton* m_btn_run;
	QPushButton* m_btn_pause;
	u32 m_item_count;
	QComboBox* m_choice_units;

	u64 m_threads_created = 0;
	u64 m_threads_deleted = 0;
	u32 m_last_pc = -1;
	u32 m_last_stat = 0;

	bool timerIsActive = false;
	QTimer* update;

public:
	std::weak_ptr<cpu_thread> cpu;

public:
	explicit DebuggerFrame(QWidget *parent = 0);

	void UpdateUI();
	void UpdateUnitList();

	u32 GetPc() const;
	u32 CentrePc(u32 pc) const;
	void OnSelectUnit();
	//void resizeEvent(QResizeEvent* event);
	void DoUpdate();
	void ShowAddr(u32 addr);
	void WriteRegs();

	void OnUpdate();
	void Show_Val();
	void Show_PC();
	void DoRun();
	void DoPause();
	void DoStep();
	void keyPressEvent(QKeyEvent* event);
	void mouseDoubleClickEvent(QMouseEvent* event);

	void wheelEvent(QWheelEvent* event);
	bool IsBreakPoint(u32 pc);
	void AddBreakPoint(u32 pc);
	void RemoveBreakPoint(u32 pc);

protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event);

signals:
	void DebugFrameClosed();

public slots:
	void ToggleUpdateTimer();
};

#endif // DEBUGGERFRAME_H
