#include "welcome_dialog.h"
#include "ui_welcome_dialog.h"

#include "gui_settings.h"
#include "shortcut_utils.h"
#include "qt_utils.h"

#include "Utilities/File.h"

#include <QPushButton>
#include <QCheckBox>
#include <QSvgWidget>

welcome_dialog::welcome_dialog(std::shared_ptr<gui_settings> gui_settings, bool is_manual_show, QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::welcome_dialog)
	, m_gui_settings(std::move(gui_settings))
{
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlag(Qt::WindowCloseButtonHint, false); // disable the close button shown on the dialog's top right corner
	layout()->setSizeConstraint(QLayout::SetFixedSize);

	ui->icon_label->load(QStringLiteral(":/rpcs3.svg"));

	ui->label_desc->setText(tr(
		R"(
			<p style="white-space: nowrap;">
				RPCS3 is an open-source Sony PlayStation 3 emulator and debugger.<br>
				It is written in C++ for Windows, Linux, FreeBSD and MacOS funded with <a %0 href="https://rpcs3.net/patreon">Patreon</a>.<br>
				Our developers and contributors are always working hard to ensure this project is the best that it can be.<br>
				There are still plenty of implementations to make and optimizations to do.
			</p>
		)"
	).arg(gui::utils::get_link_style()));

	ui->label_info->setText(tr(
		R"(
			<p style="white-space: nowrap;">
				To get started, you must first install the <span style="font-weight:600;">PlayStation 3 firmware</span>.<br>
				Please refer to the <a %0 href="https://rpcs3.net/quickstart">Quickstart</a> guide found on the official website for further information.<br>
				If you have any further questions, please refer to the <a %0 href="https://rpcs3.net/faq">FAQ</a>.<br>
				Otherwise, further discussion and support can be found on the <a %0 href="https://forums.rpcs3.net">Forums</a> or on our <a %0 href="https://discord.me/RPCS3">Discord</a> server.
			</p>
		)"
	).arg(gui::utils::get_link_style()));

#ifdef __APPLE__
	ui->create_applications_menu_shortcut->setText(tr("&Create Launchpad shortcut"));
	ui->use_dark_theme->setVisible(false);
	ui->use_dark_theme->setEnabled(false);
#else
#ifndef _WIN32
	ui->create_applications_menu_shortcut->setText(tr("&Create Application Menu shortcut"));
#endif

	ui->use_dark_theme->setVisible(!is_manual_show);
	ui->use_dark_theme->setEnabled(!is_manual_show);
	ui->use_dark_theme->setChecked(gui::utils::dark_mode_active());
#endif

	ui->accept->setEnabled(is_manual_show);
	ui->reject->setVisible(!is_manual_show);
	ui->i_have_read->setVisible(!is_manual_show);
	ui->i_have_read->setChecked(is_manual_show);
	ui->show_at_startup->setChecked(m_gui_settings->GetValue(gui::ib_show_welcome).toBool());

	if (!is_manual_show)
	{
		connect(ui->i_have_read, &QCheckBox::clicked, this, [this](bool checked)
		{
			ui->accept->setEnabled(checked);
			ui->reject->setEnabled(!checked);
		});
	}

	connect(ui->show_at_startup, &QCheckBox::clicked, this, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::ib_show_welcome, QVariant(checked));
	});

	connect(ui->accept, &QPushButton::clicked, this, &QDialog::accept); // trigger "accept" signal (setting also dialog's result code to QDialog::Accepted)
	connect(ui->reject, &QPushButton::clicked, this, &QDialog::reject); // trigger "reject" signal (setting also dialog's result code to QDialog::Rejected)

	connect(this, &QDialog::accepted, this, [this]() // "accept" signal's event handler
	{
		if (ui->create_desktop_shortcut->isChecked())
		{
			gui::utils::create_shortcut("RPCS3", "", "", "RPCS3", ":/rpcs3.svg", fs::get_temp_dir(), gui::utils::shortcut_location::desktop);
		}

		if (ui->create_applications_menu_shortcut->isChecked())
		{
			gui::utils::create_shortcut("RPCS3", "", "", "RPCS3", ":/rpcs3.svg", fs::get_temp_dir(), gui::utils::shortcut_location::applications);
		}

		if (ui->use_dark_theme->isChecked() && ui->use_dark_theme->isEnabled()) // if checked and also on initial welcome dialog
		{
			m_gui_settings->SetValue(gui::m_currentStylesheet, gui::DarkStylesheet);
		}
	});

	connect(this, &QDialog::rejected, this, [this]() // "reject" signal's event handler
	{
		// if the agreement on RPCS3's usage conditions was not accepted by the user, always display the initial welcome dialog at next startup
		m_gui_settings->SetValue(gui::ib_show_welcome, QVariant(true));
	});
}

welcome_dialog::~welcome_dialog()
{
}
