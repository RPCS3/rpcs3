#pragma once

#include "util/types.hpp"

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>

#include <memory>

class CPUDisAsm;
class cpu_thread;

class register_editor_dialog : public QDialog
{
	Q_OBJECT

	CPUDisAsm* m_disasm;
	QComboBox* m_register_combo;
	QLineEdit* m_value_line;

public:
	std::weak_ptr<cpu_thread> cpu;

public:
	register_editor_dialog(QWidget *parent, const std::shared_ptr<cpu_thread>& _cpu, CPUDisAsm* _disasm);

private:
	void OnOkay(const std::shared_ptr<cpu_thread>& _cpu);

private Q_SLOTS:
	void updateRegister(int reg);
};
