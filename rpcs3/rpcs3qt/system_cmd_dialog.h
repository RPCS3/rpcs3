#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>

class system_cmd_dialog : public QDialog
{
	Q_OBJECT

public:
	system_cmd_dialog(QWidget* parent = nullptr);
	~system_cmd_dialog();

private Q_SLOTS:
	void send_command();
	void send_custom_command();

private:
	void send_cmd(qulonglong status, qulonglong param) const;
	qulonglong hex_value(QString text, bool& ok) const;

	QLineEdit* m_value_input = nullptr;
	QLineEdit* m_custom_command_input = nullptr;
	QComboBox* m_command_box = nullptr;
};
