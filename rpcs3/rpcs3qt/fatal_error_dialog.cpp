#include "fatal_error_dialog.h"

#include <QLayout>
#include <QTextDocument>
#include <QIcon>

fatal_error_dialog::fatal_error_dialog(std::string_view text, bool is_html) : QMessageBox()
{
	const QString msg = QString::fromUtf8(text.data(), text.size());

#ifndef __APPLE__
	setWindowIcon(QIcon(":/rpcs3.ico"));
#endif
	setWindowTitle(tr("RPCS3: Fatal Error"));
	setIcon(QMessageBox::Icon::Critical);
	setTextFormat(Qt::TextFormat::RichText);
	setText(QString(R"(
			<style>
				p {white-space: nowrap;}
			</style>
			<p>
				%1<br>
				%2<br>
				<a href='https://github.com/RPCS3/rpcs3/wiki/How-to-ask-for-Support'>https://github.com/RPCS3/rpcs3/wiki/How-to-ask-for-Support</a><br>
				%3<br>
			</p>
			)")
		.arg(is_html ? msg : Qt::convertFromPlainText(msg))
		.arg(tr("HOW TO REPORT ERRORS:"))
		.arg(tr("Please, don't send incorrect reports. Thanks for understanding.")));
	layout()->setSizeConstraint(QLayout::SetFixedSize);
}
