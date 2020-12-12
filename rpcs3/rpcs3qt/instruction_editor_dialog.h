#pragma once

#include "stdafx.h"

#include <QDialog>
#include <QLineEdit>
#include <QLabel>

class CPUDisAsm;
class cpu_thread;

class instruction_editor_dialog : public QDialog
{
	Q_OBJECT

private:
	u32 m_pc;
	u8* m_cpu_offset;
	CPUDisAsm* m_disasm;
	QLineEdit* m_instr;
	QLabel* m_preview;

public:
	std::weak_ptr<cpu_thread> cpu;

	instruction_editor_dialog(QWidget *parent, u32 _pc, const std::shared_ptr<cpu_thread>& _cpu, CPUDisAsm* _disasm);

	void updatePreview();
};
