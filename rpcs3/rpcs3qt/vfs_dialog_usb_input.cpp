#include "vfs_dialog_usb_input.h"
#include "gui_settings.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "Emu/vfs_config.h"

vfs_dialog_usb_input::vfs_dialog_usb_input(const QString& name, const cfg::device_info& default_info, cfg::device_info* info, std::shared_ptr<gui_settings> _gui_settings, QWidget* parent)
	: QDialog(parent), m_gui_settings(std::move(_gui_settings)), m_gui_save(gui::fs_dev_usb_list)
{
	ensure(!!info);
	ensure(!name.isEmpty());
	ensure(name.back() >= '0' && name.back() <= '7');

	setWindowTitle(tr("Edit %0").arg(name));
	setObjectName("vfs_dialog_usb_input");

	m_gui_save.name.replace('X', name.back());

	// Create path widget
	m_path_widget = new vfs_dialog_path_widget(name, QString::fromStdString(info->path), QString::fromStdString(default_info.path), m_gui_save, m_gui_settings);
	m_path_widget->layout()->setContentsMargins(0, 0, 0, 0);

	// Create buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Apply | QDialogButtonBox::RestoreDefaults);
	buttons->button(QDialogButtonBox::RestoreDefaults)->setText(tr("Reset All"));
	buttons->button(QDialogButtonBox::Apply)->setDefault(true);

	connect(buttons, &QDialogButtonBox::clicked, this, [this, buttons, info](QAbstractButton* button)
	{
		if (button == buttons->button(QDialogButtonBox::Apply))
		{
			m_gui_settings->SetValue(m_gui_save, m_path_widget->get_dir_list());

			info->path   = m_path_widget->get_selected_path();
			info->vid    = m_vid_edit->text().toStdString();
			info->pid    = m_pid_edit->text().toStdString();
			info->serial = m_serial_edit->text().toStdString();

			accept();
		}
		else if (button == buttons->button(QDialogButtonBox::RestoreDefaults))
		{
			if (QMessageBox::question(this, tr("Confirm Reset"), tr("Reset all entries and file system directories?")) != QMessageBox::Yes)
				return;

			m_path_widget->reset();
			m_vid_edit->setText("");
			m_pid_edit->setText("");
			m_serial_edit->setText("");
		}
		else if (button == buttons->button(QDialogButtonBox::Cancel))
		{
			reject();
		}
	});

	m_vid_edit = new QLineEdit;
	m_vid_edit->setMaxLength(4);
	m_vid_edit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[a-fA-F0-9]*$"), this)); // HEX only
	m_vid_edit->setText(QString::fromStdString(info->vid));

	m_pid_edit = new QLineEdit;
	m_pid_edit->setMaxLength(4);
	m_pid_edit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[a-fA-F0-9]*$"), this)); // HEX only
	m_pid_edit->setText(QString::fromStdString(info->pid));

	m_serial_edit = new QLineEdit;
	m_serial_edit->setMaxLength(64); // Max length defined in sys_fs
	m_serial_edit->setText(QString::fromStdString(info->serial));

	QVBoxLayout* vbox_left = new QVBoxLayout;
	vbox_left->addWidget(new QLabel(tr("Vendor ID:")));
	vbox_left->addWidget(new QLabel(tr("Product ID:")));
	vbox_left->addWidget(new QLabel(tr("Serial:")));

	QVBoxLayout* vbox_right = new QVBoxLayout;
	vbox_right->addWidget(m_vid_edit);
	vbox_right->addWidget(m_pid_edit);
	vbox_right->addWidget(m_serial_edit);

	QHBoxLayout* hbox = new QHBoxLayout;
	hbox->addLayout(vbox_left);
	hbox->addLayout(vbox_right);

	QVBoxLayout* vbox = new QVBoxLayout;
	vbox->addWidget(m_path_widget);
	vbox->addLayout(hbox);
	vbox->addWidget(buttons);

	setLayout(vbox);

	buttons->button(QDialogButtonBox::Apply)->setFocus();
}
