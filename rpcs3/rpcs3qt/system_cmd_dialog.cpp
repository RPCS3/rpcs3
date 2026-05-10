#include "stdafx.h"
#include "system_cmd_dialog.h"

#include "Emu/System.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/Cell/Modules/cellNetCtl.h"
#include "Emu/Cell/Modules/cellOskDialog.h"

#include <QPushButton>
#include <QFontDatabase>
#include <QRegularExpressionValidator>
#include <QGroupBox>
#include <QMessageBox>
#include <QVBoxLayout>

LOG_CHANNEL(gui_log, "GUI");

system_cmd_dialog::system_cmd_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("System Commands | %1").arg(QString::fromStdString(Emu.GetTitleAndTitleID())));
	setObjectName("system_cmd_dialog");
	setAttribute(Qt::WA_DeleteOnClose);

	QPushButton* button_send = new QPushButton(tr("Send Command"), this);
	connect(button_send, &QAbstractButton::clicked, this, &system_cmd_dialog::send_command);

	QPushButton* button_send_custom = new QPushButton(tr("Send Custom Command"), this);
	connect(button_send_custom, &QAbstractButton::clicked, this, &system_cmd_dialog::send_custom_command);

	const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

	m_value_input = new QLineEdit();
	m_value_input->setFont(mono);
	m_value_input->setMaxLength(18);
	m_value_input->setValidator(new QRegularExpressionValidator(QRegularExpression("^(0[xX])?0*[a-fA-F0-9]{0,8}$"), this));
	m_value_input->setPlaceholderText(QString("0x%1").arg(0, 16, 16, QChar('0')));

	m_custom_command_input = new QLineEdit();
	m_custom_command_input->setFont(mono);
	m_custom_command_input->setMaxLength(18);
	m_custom_command_input->setValidator(new QRegularExpressionValidator(QRegularExpression("^(0[xX])?0*[a-fA-F0-9]{0,8}$"), this));
	m_custom_command_input->setPlaceholderText(QString("0x%1").arg(0, 16, 16, QChar('0')));

	m_command_box = new QComboBox();

	const std::map<u64, QString> commands
	{
		{ CELL_SYSUTIL_REQUEST_EXITGAME, "CELL_SYSUTIL_REQUEST_EXITGAME " },
		{ CELL_SYSUTIL_DRAWING_BEGIN, "CELL_SYSUTIL_DRAWING_BEGIN" },
		{ CELL_SYSUTIL_DRAWING_END, "CELL_SYSUTIL_DRAWING_END" },
		{ CELL_SYSUTIL_SYSTEM_MENU_OPEN, "CELL_SYSUTIL_SYSTEM_MENU_OPEN" },
		{ CELL_SYSUTIL_SYSTEM_MENU_CLOSE, "CELL_SYSUTIL_SYSTEM_MENU_CLOSE" },
		{ CELL_SYSUTIL_BGMPLAYBACK_PLAY, "CELL_SYSUTIL_BGMPLAYBACK_PLAY" },
		{ CELL_SYSUTIL_BGMPLAYBACK_STOP, "CELL_SYSUTIL_BGMPLAYBACK_STOP" },
		{ CELL_SYSUTIL_NP_INVITATION_SELECTED, "CELL_SYSUTIL_NP_INVITATION_SELECTED" },
		{ CELL_SYSUTIL_NP_DATA_MESSAGE_SELECTED, "CELL_SYSUTIL_NP_DATA_MESSAGE_SELECTED" },
		{ CELL_SYSUTIL_SYSCHAT_START, "CELL_SYSUTIL_SYSCHAT_START" },
		{ CELL_SYSUTIL_SYSCHAT_STOP, "CELL_SYSUTIL_SYSCHAT_STOP" },
		{ CELL_SYSUTIL_SYSCHAT_VOICE_STREAMING_RESUMED, "CELL_SYSUTIL_SYSCHAT_VOICE_STREAMING_RESUMED" },
		{ CELL_SYSUTIL_SYSCHAT_VOICE_STREAMING_PAUSED, "CELL_SYSUTIL_SYSCHAT_VOICE_STREAMING_PAUSED" },
		{ CELL_SYSUTIL_NET_CTL_NETSTART_LOADED, "CELL_SYSUTIL_NET_CTL_NETSTART_LOADED" },
		{ CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED, "CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED" },
		{ CELL_SYSUTIL_NET_CTL_NETSTART_UNLOADED, "CELL_SYSUTIL_NET_CTL_NETSTART_UNLOADED" },
		{ CELL_SYSUTIL_OSKDIALOG_LOADED, "CELL_SYSUTIL_OSKDIALOG_LOADED" },
		{ CELL_SYSUTIL_OSKDIALOG_FINISHED, "CELL_SYSUTIL_OSKDIALOG_FINISHED" },
		{ CELL_SYSUTIL_OSKDIALOG_UNLOADED, "CELL_SYSUTIL_OSKDIALOG_UNLOADED" },
		{ CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED, "CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED" },
		{ CELL_SYSUTIL_OSKDIALOG_INPUT_CANCELED, "CELL_SYSUTIL_OSKDIALOG_INPUT_CANCELED" },
		{ CELL_SYSUTIL_OSKDIALOG_INPUT_DEVICE_CHANGED, "CELL_SYSUTIL_OSKDIALOG_INPUT_DEVICE_CHANGED" },
		{ CELL_SYSUTIL_OSKDIALOG_DISPLAY_CHANGED, "CELL_SYSUTIL_OSKDIALOG_DISPLAY_CHANGED" },
	};

	for (const auto& [status, text] : commands)
	{
		m_command_box->addItem(text, static_cast<qulonglong>(status));
	}

	QGroupBox* commands_group = new QGroupBox(tr("Select Command"));
	QHBoxLayout* hbox_commands = new QHBoxLayout();
	hbox_commands->addWidget(m_command_box);
	commands_group->setLayout(hbox_commands);

	QGroupBox* custom_commands_group = new QGroupBox(tr("Select Custom Command"));
	QHBoxLayout* hbox_custom_commands = new QHBoxLayout();
	hbox_custom_commands->addWidget(m_custom_command_input);
	custom_commands_group->setLayout(hbox_custom_commands);

	QGroupBox* value_group = new QGroupBox(tr("Select Value"));
	QHBoxLayout* hbox_value = new QHBoxLayout();
	hbox_value->addWidget(m_value_input);
	value_group->setLayout(hbox_value);

	QHBoxLayout* hbox_buttons = new QHBoxLayout();
	hbox_buttons->addWidget(button_send);
	hbox_buttons->addWidget(button_send_custom);

	QVBoxLayout* vbox_panel = new QVBoxLayout();
	vbox_panel->addWidget(commands_group);
	vbox_panel->addWidget(custom_commands_group);
	vbox_panel->addWidget(value_group);
	vbox_panel->addLayout(hbox_buttons);
	setLayout(vbox_panel);
}

system_cmd_dialog::~system_cmd_dialog()
{
}

void system_cmd_dialog::send_command()
{
	bool ok = false;

	const qulonglong status = m_command_box->currentData().toULongLong(&ok);
	if (!ok)
	{
		gui_log.error("system_cmd_dialog::send_command: command can not be converted to qulonglong: %s", m_command_box->currentText());
		QMessageBox::warning(this, tr("Listen!"), tr("The selected command is bugged.\nPlease contact a developer."));
		return;
	}

	const qulonglong param = hex_value(m_value_input->text(), ok);
	if (!ok)
	{
		gui_log.error("system_cmd_dialog::send_command: value can not be converted to qulonglong: %s", m_value_input->text());
		QMessageBox::information(this, tr("Listen!"), tr("Please select a proper value first."));
		return;
	}

	send_cmd(status, param);
}

void system_cmd_dialog::send_custom_command()
{
	bool ok = false;

	const qulonglong status = hex_value(m_custom_command_input->text(), ok);
	if (!ok)
	{
		gui_log.error("system_cmd_dialog::send_custom_command: command can not be converted to qulonglong: %s", m_custom_command_input->text());
		QMessageBox::information(this, tr("Listen!"), tr("Please select a proper custom command first."));
		return;
	}

	const qulonglong param = hex_value(m_value_input->text(), ok);
	if (!ok)
	{
		gui_log.error("system_cmd_dialog::send_custom_command: value can not be converted to qulonglong: %s", m_value_input->text());
		QMessageBox::information(this, tr("Listen!"), tr("Please select a proper value first."));
		return;
	}

	send_cmd(status, param);
}

void system_cmd_dialog::send_cmd(qulonglong status, qulonglong param) const
{
	if (Emu.IsStopped())
	{
		return;
	}

	gui_log.warning("User triggered sysutil_send_system_cmd(status=0x%x, param=0x%x) in system_cmd_dialog", status, param);

	if (s32 ret = sysutil_send_system_cmd(status, param); ret < 0)
	{
		gui_log.error("sysutil_send_system_cmd(status=0x%x, param=0x%x) failed with 0x%x", status, param, ret);
	}
}

qulonglong system_cmd_dialog::hex_value(QString text, bool& ok) const
{
	if (!text.startsWith("0x")) text.prepend("0x");
	return text.toULongLong(&ok, 16);
}
