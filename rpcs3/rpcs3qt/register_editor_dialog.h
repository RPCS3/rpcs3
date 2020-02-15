#pragma once

#include "stdafx.h"
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

class register_editor_dialog : public QDialog
{
	Q_OBJECT

	u32 m_pc;
	CPUDisAsm* m_disasm;
	QComboBox* m_register_combo;
	QLineEdit* m_value_line;

public:
	std::weak_ptr<cpu_thread> cpu;

public:
	register_editor_dialog(QWidget *parent, u32 _pc, const std::shared_ptr<cpu_thread>& _cpu, CPUDisAsm* _disasm);

private:
	void OnOkay(const std::shared_ptr<cpu_thread>& _cpu);

private Q_SLOTS:
	void updateRegister(const QString& text);
};
