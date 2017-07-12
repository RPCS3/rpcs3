#include "vfs_dialog.h"

#include "Emu/System.h"

#include "vfs_dialog_tab.h"

#include <QVBoxLayout>
#include <QPushButton>

inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }

vfs_dialog::vfs_dialog(QWidget* parent) : QDialog(parent),
	m_gui_settings(), m_emu_settings("")
{
	tabs = new QTabWidget();
	tabs->setUsesScrollButtons(false);

	// Create tabs
	vfs_dialog_tab* emulator_tab = new vfs_dialog_tab({ "$(EmulatorDir)", emu_settings::emulatorLocation, GUI::fs_emulator_dir_list, &g_cfg.vfs.emulator_dir },
		&m_gui_settings, &m_emu_settings, this);

	vfs_dialog_tab* dev_hdd0_tab = new vfs_dialog_tab({ "dev_hdd0", emu_settings::dev_hdd0Location, GUI::fs_dev_hdd0_list, &g_cfg.vfs.dev_hdd0 },
		&m_gui_settings, &m_emu_settings, this);

	vfs_dialog_tab* dev_hdd1_tab = new vfs_dialog_tab({ "dev_hdd1", emu_settings::dev_hdd1Location, GUI::fs_dev_hdd1_list, &g_cfg.vfs.dev_hdd1 },
		&m_gui_settings, &m_emu_settings, this);

	vfs_dialog_tab* dev_flash_tab = new vfs_dialog_tab({ "dev_flash", emu_settings::dev_flashLocation, GUI::fs_dev_flash_list, &g_cfg.vfs.dev_flash },
		&m_gui_settings, &m_emu_settings, this);

	vfs_dialog_tab* dev_usb000_tab = new vfs_dialog_tab({ "dev_usb000", emu_settings::dev_usb000Location, GUI::fs_dev_usb000_list, &g_cfg.vfs.dev_usb000 },
		&m_gui_settings, &m_emu_settings, this);

	tabs->addTab(emulator_tab, "$(EmulatorDir)");
	tabs->addTab(dev_hdd0_tab, "dev_hdd0");
	tabs->addTab(dev_hdd1_tab, "dev_hdd1");
	tabs->addTab(dev_flash_tab, "dev_flash");
	tabs->addTab(dev_usb000_tab, "dev_usb000");

	// Create buttons
	QPushButton* addDir = new QPushButton(tr("Add New Directory"));
	connect(addDir, &QAbstractButton::pressed, [this]() {
		static_cast<vfs_dialog_tab*>(tabs->currentWidget())->AddNewDirectory();
	});
	QPushButton* reset = new QPushButton(tr("Reset"));
	connect(reset, &QAbstractButton::pressed, [this]() {
		static_cast<vfs_dialog_tab*>(tabs->currentWidget())->Reset();
	});
	QPushButton* resetAll = new QPushButton(tr("Reset All"));
	connect(resetAll, &QAbstractButton::pressed, [this]() {
		for (int i = 0; i < tabs->count(); ++i)
		{
			static_cast<vfs_dialog_tab*>(tabs->widget(i))->Reset();
		}
	});
	QPushButton* okay = new QPushButton(tr("Okay"));
	okay->setAutoDefault(true);
	okay->setDefault(true);

	connect(okay, &QAbstractButton::pressed, [this]() {
		for (int i = 0; i < tabs->count(); ++i)
		{
			static_cast<vfs_dialog_tab*>(tabs->widget(i))->SaveSettings();
		}
		accept();
	});

	QHBoxLayout* buttons = new QHBoxLayout;
	buttons->addWidget(addDir);
	buttons->addWidget(reset);
	buttons->addWidget(resetAll);
	buttons->addStretch();
	buttons->addWidget(okay);

	QVBoxLayout* vbox = new QVBoxLayout;
	vbox->addWidget(tabs);
	vbox->addLayout(buttons);

	setLayout(vbox);
	setWindowTitle("Virtual File System");
}
