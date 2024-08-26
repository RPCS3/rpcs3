#pragma once

#include "util/types.hpp"

#include <QDialog>
#include <QLineEdit>
#include <QLabel>

class CPUDisAsm;
class cpu_thread;
class QCheckBox;

class instruction_editor_dialog : public QDialog
{
	Q_OBJECT

private:
	u32 m_pc;
	u8* m_cpu_offset;
	std::shared_ptr<CPUDisAsm> m_disasm; // shared in order to allow an incomplete type
	QLineEdit* m_instr;
	QLabel* m_preview;
	QLabel* m_func_info = nullptr;
	QCheckBox* m_apply_for_spu_group = nullptr;

	const std::function<cpu_thread*()> m_get_cpu;

public:
	instruction_editor_dialog(QWidget *parent, u32 _pc, CPUDisAsm* _disasm, std::function<cpu_thread*()> func);

	void updatePreview() const;
};
