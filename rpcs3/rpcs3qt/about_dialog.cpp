#include "about_dialog.h"
#include "ui_about_dialog.h"

#include "rpcs3_version.h"
#include "qt_utils.h"

#include <QDesktopServices>
#include <QUrl>
#include <QSvgWidget>

about_dialog::about_dialog(QWidget* parent) : QDialog(parent), ui(new Ui::about_dialog)
{
	ui->setupUi(this);

	ui->close->setDefault(true);
	ui->icon->load(QStringLiteral(":/rpcs3.svg"));
	ui->version->setText(tr("RPCS3 Version: %1").arg(QString::fromStdString(rpcs3::get_verbose_version())));
	ui->description->setText(tr(
		R"(
			<p style="white-space: nowrap;">
				RPCS3 is an open-source Sony PlayStation 3 emulator and debugger.<br>
				It is written in C++ for Windows, Linux, FreeBSD and MacOS funded with <a %0 href="https://www.patreon.com/Nekotekina">Patreon</a>.<br>
				Our developers and contributors are always working hard to ensure this project is the best that it can be.<br>
				There are still plenty of implementations to make and optimizations to do.
			</p>
		)"
	).arg(gui::utils::get_link_style()));

	// Events
	connect(ui->gitHub, &QPushButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://www.github.com/RPCS3")); });
	connect(ui->website, &QPushButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://www.rpcs3.net")); });
	connect(ui->forum, &QPushButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://forums.rpcs3.net")); });
	connect(ui->patreon, &QPushButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://www.patreon.com/Nekotekina")); });
	connect(ui->discord, &QPushButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://discord.me/RPCS3")); });
	connect(ui->wiki, &QPushButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://wiki.rpcs3.net/index.php?title=Main_Page")); });
	connect(ui->close, &QPushButton::clicked, this, &QWidget::close);
}

about_dialog::~about_dialog()
{
}
