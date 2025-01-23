#include "vfs_dialog.h"
#include "vfs_dialog_tab.h"
#include "vfs_dialog_usb_tab.h"
#include "gui_settings.h"

#include <QTabWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>
#include <QVBoxLayout>

#include "Emu/System.h"
#include "Emu/vfs_config.h"

vfs_dialog::vfs_dialog(std::shared_ptr<gui_settings> _gui_settings, QWidget* parent)
	: QDialog(parent), m_gui_settings(std::move(_gui_settings))
{
	setWindowTitle(tr("Virtual File System"));
	setObjectName("vfs_dialog");

	QTabWidget* tabs = new QTabWidget();
	tabs->setUsesScrollButtons(false);

	g_cfg_vfs.load();

	// Create tabs
	vfs_dialog_tab* emulator_tab = new vfs_dialog_tab("$(EmulatorDir)", gui::fs_emulator_dir_list, &g_cfg_vfs.emulator_dir, m_gui_settings, this);
	vfs_dialog_tab* dev_hdd0_tab = new vfs_dialog_tab("dev_hdd0", gui::fs_dev_hdd0_list, &g_cfg_vfs.dev_hdd0, m_gui_settings, this);
	vfs_dialog_tab* dev_hdd1_tab = new vfs_dialog_tab("dev_hdd1", gui::fs_dev_hdd1_list, &g_cfg_vfs.dev_hdd1, m_gui_settings, this);
	vfs_dialog_tab* dev_flash_tab = new vfs_dialog_tab("dev_flash", gui::fs_dev_flash_list, &g_cfg_vfs.dev_flash, m_gui_settings, this);
	vfs_dialog_tab* dev_flash2_tab = new vfs_dialog_tab("dev_flash2", gui::fs_dev_flash2_list, &g_cfg_vfs.dev_flash2, m_gui_settings, this);
	vfs_dialog_tab* dev_flash3_tab = new vfs_dialog_tab("dev_flash3", gui::fs_dev_flash3_list, &g_cfg_vfs.dev_flash3, m_gui_settings, this);
	vfs_dialog_tab* dev_bdvd_tab = new vfs_dialog_tab("dev_bdvd", gui::fs_dev_bdvd_list, &g_cfg_vfs.dev_bdvd, m_gui_settings, this);
	vfs_dialog_usb_tab* dev_usb_tab = new vfs_dialog_usb_tab(&g_cfg_vfs.dev_usb, m_gui_settings, this);
	vfs_dialog_tab* games_tab = new vfs_dialog_tab("games", gui::fs_games_list, &g_cfg_vfs.games_dir, m_gui_settings, this);

	tabs->addTab(emulator_tab, "$(EmulatorDir)");
	tabs->addTab(dev_hdd0_tab, "dev_hdd0");
	tabs->addTab(dev_hdd1_tab, "dev_hdd1");
	tabs->addTab(dev_flash_tab, "dev_flash");
	tabs->addTab(dev_flash2_tab, "dev_flash2");
	tabs->addTab(dev_flash3_tab, "dev_flash3");
	tabs->addTab(dev_bdvd_tab, "dev_bdvd");
	tabs->addTab(dev_usb_tab, "dev_usb");
	tabs->addTab(games_tab, "games");

	// Create buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Save | QDialogButtonBox::RestoreDefaults);
	buttons->button(QDialogButtonBox::RestoreDefaults)->setText(tr("Reset Directories"));
	buttons->button(QDialogButtonBox::Save)->setDefault(true);

	connect(buttons, &QDialogButtonBox::clicked, this, [this, buttons, tabs](QAbstractButton* button)
	{
		if (button == buttons->button(QDialogButtonBox::RestoreDefaults))
		{
			if (QMessageBox::question(this, tr("Confirm Reset"), tr("Reset all file system directories?")) != QMessageBox::Yes)
				return;

			for (int i = 0; i < tabs->count(); ++i)
			{
				if (tabs->tabText(i) == "dev_usb")
				{
					static_cast<vfs_dialog_usb_tab*>(tabs->widget(i))->reset();
				}
				else
				{
					static_cast<vfs_dialog_tab*>(tabs->widget(i))->reset();
				}
			}
		}
		else if (button == buttons->button(QDialogButtonBox::Save))
		{
			for (int i = 0; i < tabs->count(); ++i)
			{
				if (tabs->tabText(i) == "dev_usb")
				{
					static_cast<vfs_dialog_usb_tab*>(tabs->widget(i))->set_settings();
				}
				else
				{
					static_cast<vfs_dialog_tab*>(tabs->widget(i))->set_settings();
				}
			}

			g_cfg_vfs.save();

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

	buttons->button(QDialogButtonBox::Save)->setFocus();
}


#include <QPushButton>
#include <QVBoxLayout>

void vfs_dialog::setup_disc_features()
{
    // Create a layout for disc-related actions
    QVBoxLayout* disc_layout = new QVBoxLayout;

    // Add "Mount Disc" button
    QPushButton* mount_disc_button = new QPushButton(tr("Mount Disc"), this);
    connect(mount_disc_button, &QPushButton::clicked, this, &vfs_dialog::on_mount_disc_clicked);
    disc_layout->addWidget(mount_disc_button);

    // Add "Mount ISO" button
    QPushButton* mount_iso_button = new QPushButton(tr("Mount ISO"), this);
    connect(mount_iso_button, &QPushButton::clicked, this, &vfs_dialog::on_mount_iso_clicked);
    disc_layout->addWidget(mount_iso_button);

    // Add "Unmount Disc" button
    QPushButton* unmount_disc_button = new QPushButton(tr("Unmount Disc"), this);
    connect(unmount_disc_button, &QPushButton::clicked, this, &vfs_dialog::on_unmount_disc_clicked);
    disc_layout->addWidget(unmount_disc_button);

    // Add the layout to the VFS dialog
    layout()->addLayout(disc_layout);
}

// Slot for "Mount Disc" button
void vfs_dialog::on_mount_disc_clicked()
{
    QString disc_path = QFileDialog::getExistingDirectory(this, tr("Select Disc Path"));
    if (!disc_path.isEmpty())
    {
        if (!vfs::mount("/mnt/ps3_disc", disc_path.toStdString()))
        {
            QMessageBox::critical(this, tr("Error"), tr("Failed to mount the disc."));
        }
        else
        {
            QMessageBox::information(this, tr("Success"), tr("Disc mounted successfully."));
        }
    }
}

// Slot for "Mount ISO" button
void vfs_dialog::on_mount_iso_clicked()
{
    QString iso_path = QFileDialog::getOpenFileName(this, tr("Select ISO File"), QString(), tr("ISO Files (*.iso)"));
    if (!iso_path.isEmpty())
    {
        if (!vfs::mount_iso("/mnt/ps3_iso", iso_path.toStdString()))
        {
            QMessageBox::critical(this, tr("Error"), tr("Failed to mount the ISO."));
        }
        else
        {
            QMessageBox::information(this, tr("Success"), tr("ISO mounted successfully."));
        }
    }
}

// Slot for "Unmount Disc" button
void vfs_dialog::on_unmount_disc_clicked()
{
    if (!vfs::unmount("/mnt/ps3_disc"))
    {
        QMessageBox::critical(this, tr("Error"), tr("Failed to unmount the disc."));
    }
    else
    {
        QMessageBox::information(this, tr("Success"), tr("Disc unmounted successfully."));
    }
}


#include <QLabel>
#include <QVBoxLayout>
#include <QTableWidget>

void vfs_dialog::setup_metadata_display()
{
    // Create a layout for the metadata section
    QVBoxLayout* metadata_layout = new QVBoxLayout;

    // Add a label for metadata title
    QLabel* metadata_label = new QLabel(tr("Game Metadata"), this);
    metadata_layout->addWidget(metadata_label);

    // Add a table for displaying metadata
    m_metadata_table = new QTableWidget(this);
    m_metadata_table->setColumnCount(2);
    m_metadata_table->setHorizontalHeaderLabels({tr("Field"), tr("Value")});
    metadata_layout->addWidget(m_metadata_table);

    // Add the layout to the VFS dialog
    layout()->addLayout(metadata_layout);
}

// Populate metadata table
void vfs_dialog::update_metadata_display(const std::string& param_sfo_path)
{
    // Extract metadata using vfs::parse_param_sfo
    std::map<std::string, std::string> metadata;
    if (!vfs::parse_param_sfo(param_sfo_path, metadata))
    {
        QMessageBox::critical(this, tr("Error"), tr("Failed to parse game metadata."));
        return;
    }

    // Clear existing metadata
    m_metadata_table->setRowCount(0);

    // Populate the table with extracted metadata
    int row = 0;
    for (const auto& [field, value] : metadata)
    {
        m_metadata_table->insertRow(row);
        m_metadata_table->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(field)));
        m_metadata_table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(value)));
        ++row;
    }
}

void vfs_dialog::on_mount_disc_clicked()
{
    QString disc_path = QFileDialog::getExistingDirectory(this, tr("Select Disc Path"));
    if (!disc_path.isEmpty())
    {
        if (!vfs::mount("/mnt/ps3_disc", disc_path.toStdString()))
        {
            QMessageBox::critical(this, tr("Error"), tr("Failed to mount the disc."));
        }
        else
        {
            QMessageBox::information(this, tr("Success"), tr("Disc mounted successfully."));
            update_metadata_display("/mnt/ps3_disc/PS3_GAME/PARAM.SFO");
        }
    }
}

void vfs_dialog::on_mount_iso_clicked()
{
    QString iso_path = QFileDialog::getOpenFileName(this, tr("Select ISO File"), QString(), tr("ISO Files (*.iso)"));
    if (!iso_path.isEmpty())
    {
        if (!vfs::mount_iso("/mnt/ps3_iso", iso_path.toStdString()))
        {
            QMessageBox::critical(this, tr("Error"), tr("Failed to mount the ISO."));
        }
        else
        {
            QMessageBox::information(this, tr("Success"), tr("ISO mounted successfully."));
            update_metadata_display("/mnt/ps3_iso/PS3_GAME/PARAM.SFO");
        }
    }
}
