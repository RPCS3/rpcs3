#include "about_dialog.h"
#include "ui_about_dialog.h"

#include "rpcs3_version.h"

#include <QDesktopServices>
#include <QUrl>

constexpr auto qstr = QString::fromStdString;

about_dialog::about_dialog(QWidget* parent) : QDialog(parent), ui(new Ui::about_dialog)
{
	ui->setupUi(this);

	ui->close->setDefault(true);

	ui->version->setText(tr("RPCS3 Version: %1").arg(qstr(rpcs3::get_version().to_string())));

	// Events
	connect(ui->gitHub, &QPushButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://www.github.com/RPCS3")); });
	connect(ui->website, &QPushButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://www.rpcs3.net")); });
	connect(ui->forum, &QPushButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://forums.rpcs3.net")); });
	connect(ui->patreon, &QPushButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://www.patreon.com/Nekotekina")); });
	connect(ui->close, &QPushButton::clicked, this, &QWidget::close);

	layout()->setSizeConstraint(QLayout::SetFixedSize);
}

about_dialog::~about_dialog()
{
	delete ui;
}
