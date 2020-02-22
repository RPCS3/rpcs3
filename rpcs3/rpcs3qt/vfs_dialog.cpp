#include "vfs_dialog.h"
#include "vfs_dialog_tab.h"
#include "gui_settings.h"

#include <QTabWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>
#include <QVBoxLayout>

#include "Emu/System.h"
#include "Emu/system_config.h"

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

vfs_dialog::vfs_dialog(std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, QWidget* parent)
	: QDialog(parent), m_gui_settings(std::move(guiSettings)), m_emu_settings(std::move(emuSettings))
{
	QTabWidget* tabs = new QTabWidget();
	tabs->setUsesScrollButtons(false);

	m_emu_settings->LoadSettings();

	// Create tabs
	vfs_dialog_tab* emulator_tab = new vfs_dialog_tab({ "$(EmulatorDir)", emu_settings::emulatorLocation, gui::fs_emulator_dir_list, &g_cfg.vfs.emulator_dir },
		m_gui_settings, m_emu_settings, this);

	vfs_dialog_tab* dev_hdd0_tab = new vfs_dialog_tab({ "dev_hdd0", emu_settings::dev_hdd0Location, gui::fs_dev_hdd0_list, &g_cfg.vfs.dev_hdd0 },
		m_gui_settings, m_emu_settings, this);

	vfs_dialog_tab* dev_hdd1_tab = new vfs_dialog_tab({ "dev_hdd1", emu_settings::dev_hdd1Location, gui::fs_dev_hdd1_list, &g_cfg.vfs.dev_hdd1 },
		m_gui_settings, m_emu_settings, this);

	vfs_dialog_tab* dev_flash_tab = new vfs_dialog_tab({ "dev_flash", emu_settings::dev_flashLocation, gui::fs_dev_flash_list, &g_cfg.vfs.dev_flash },
		m_gui_settings, m_emu_settings, this);

	vfs_dialog_tab* dev_usb000_tab = new vfs_dialog_tab({ "dev_usb000", emu_settings::dev_usb000Location, gui::fs_dev_usb000_list, &g_cfg.vfs.dev_usb000 },
		m_gui_settings, m_emu_settings, this);

	tabs->addTab(emulator_tab, "$(EmulatorDir)");
	tabs->addTab(dev_hdd0_tab, "dev_hdd0");
	tabs->addTab(dev_hdd1_tab, "dev_hdd1");
	tabs->addTab(dev_flash_tab, "dev_flash");
	tabs->addTab(dev_usb000_tab, "dev_usb000");

	// Create buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Save | QDialogButtonBox::RestoreDefaults);
	buttons->button(QDialogButtonBox::RestoreDefaults)->setText(tr("Reset Directories"));
	buttons->button(QDialogButtonBox::Save)->setDefault(true);

	connect(buttons, &QDialogButtonBox::clicked, [=, this](QAbstractButton* button)
	{
		if (button == buttons->button(QDialogButtonBox::RestoreDefaults))
		{
			if (QMessageBox::question(this, tr("Confirm Reset"), tr("Reset all file system directories?")) != QMessageBox::Yes)
				return;

			for (int i = 0; i < tabs->count(); ++i)
			{
				static_cast<vfs_dialog_tab*>(tabs->widget(i))->Reset();
			}
		}
		else if (button == buttons->button(QDialogButtonBox::Save))
		{
			for (int i = 0; i < tabs->count(); ++i)
			{
				static_cast<vfs_dialog_tab*>(tabs->widget(i))->SetSettings();
			}
			m_emu_settings->SaveSettings();

			// Recreate folder structure for new VFS paths
			if (Emu.IsStopped())
			{
				Emu.Init();
			}

			accept();
		}
		else if (button == buttons->button(QDialogButtonBox::Close))
		{
			reject();
		}
	});

	QVBoxLayout* vbox = new QVBoxLayout;
	vbox->addWidget(tabs);
	vbox->addWidget(buttons);

	setLayout(vbox);
	setWindowTitle(tr("Virtual File System"));
	setObjectName("vfs_dialog");

	buttons->button(QDialogButtonBox::Save)->setFocus();
}
