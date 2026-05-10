#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>

class CPUDisAsm;
class cpu_thread;

class register_editor_dialog : public QDialog
{
	Q_OBJECT

	CPUDisAsm* m_disasm;
	QComboBox* m_register_combo;
	QLineEdit* m_value_line;

	const std::function<cpu_thread*()> m_get_cpu;

public:
	register_editor_dialog(QWidget *parent, CPUDisAsm* _disasm, std::function<cpu_thread*()> func);

private:
	void OnOkay();

private Q_SLOTS:
	void updateRegister(int reg) const;
};
