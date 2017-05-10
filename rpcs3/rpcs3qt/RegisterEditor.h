#ifndef REGISTEREDITOR_H
#define REGISTEREDITOR_H

#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

class RegisterEditorDialog : public QDialog
{
	u32 pc;
	CPUDisAsm* disasm;
	QComboBox* t1_register;
	QLineEdit* t2_value;
	QLabel* t3_preview;

public:
	std::weak_ptr<cpu_thread> cpu;

public:
	RegisterEditorDialog(QWidget *parent, u32 _pc, const std::shared_ptr<cpu_thread>& _cpu, CPUDisAsm* _disasm);

	void updateRegister();
};

#endif // REGISTEREDITOR_H
