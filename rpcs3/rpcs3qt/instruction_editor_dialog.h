#pragma once

#include "stdafx.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPushButton>

class instruction_editor_dialog : public QDialog
{
	u32 m_pc;
	u32 m_cpu_offset;
	CPUDisAsm* m_disasm;
	QLineEdit* m_instr;
	QLabel* m_preview;

public:
	std::weak_ptr<cpu_thread> cpu;

public:
	instruction_editor_dialog(QWidget *parent, u32 _pc, const std::shared_ptr<cpu_thread>& _cpu, CPUDisAsm* _disasm);

	void updatePreview();
};
