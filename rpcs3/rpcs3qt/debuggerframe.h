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

class DebuggerList;

class DebuggerFrame : public QDockWidget
{
	Q_OBJECT

	QWidget* body;
	DebuggerList* m_list;
	int pSize;
	QFont mono;
	QTextEdit* m_regs;
	QPushButton* m_btn_step;
	QPushButton* m_btn_run;
	QPushButton* m_btn_pause;
	QComboBox* m_choice_units;

	u64 m_threads_created = 0;
	u64 m_threads_deleted = 0;
	u32 m_last_pc = -1;
	u32 m_last_stat = 0;

	QTimer* update;

public:
	std::unique_ptr<CPUDisAsm> m_disasm;
	std::weak_ptr<cpu_thread> cpu;

public:
	explicit DebuggerFrame(QWidget *parent = 0);

	void UpdateUI();
	void UpdateUnitList();

	u32 GetPc() const;
	u32 CentrePc(u32 pc) const;
	//void resizeEvent(QResizeEvent* event);
	void DoUpdate();
	void WriteRegs();

	void OnUpdate();

protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event);

signals:
	void DebugFrameClosed();
public slots:
	void DoStep();
private slots:
	void OnSelectUnit();
	void Show_Val();
	void Show_PC();
	void DoRun();
	void DoPause();
	void EnableUpdateTimer(bool state);
};

class DebuggerList : public QListWidget
{
	Q_OBJECT

	DebuggerFrame* m_debugFrame;

public:
	u32 m_pc;
	u32 m_item_count;

public:
	DebuggerList(DebuggerFrame* parent);
	void ShowAddr(u32 addr);

private:
	bool IsBreakPoint(u32 pc);
	void AddBreakPoint(u32 pc);
	void RemoveBreakPoint(u32 pc);

protected:
	void keyPressEvent(QKeyEvent* event);
	void mouseDoubleClickEvent(QMouseEvent* event);
	void wheelEvent(QWheelEvent* event);
};

#endif // DEBUGGERFRAME_H
