#include "about_dialog.h"
#include "ui_about_dialog.h"

#include "rpcs3_version.h"

#include <QDesktopServices>
#include <QUrl>

inline QString qstr(const std::string& _in) { return QString::fromUtf8(_in.data(), _in.size()); }

about_dialog::about_dialog(QWidget* parent) : QDialog(parent), ui(new Ui::about_dialog)
{
	ui->setupUi(this);

	ui->close->setDefault(true);

	ui->version->setText(tr("RPCS3 Version: %1").arg(qstr(rpcs3::version.to_string())));

	// Events
	connect(ui->gitHub, &QAbstractButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://www.github.com/RPCS3")); });
	connect(ui->website, &QAbstractButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://www.rpcs3.net")); });
	connect(ui->forum, &QAbstractButton::clicked, [] { QDesktopServices::openUrl(QUrl("http://www.emunewz.net/forum/forumdisplay.php?fid=172")); });
	connect(ui->patreon, &QAbstractButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://www.patreon.com/Nekotekina")); });
	connect(ui->close, &QAbstractButton::clicked, this, &QWidget::close);

	layout()->setSizeConstraint(QLayout::SetFixedSize);
}
