#include "fatal_error_dialog.h"

#include <QLayout>
#include <QTextDocument>
#include <QIcon>

fatal_error_dialog::fatal_error_dialog(std::string_view text) : QMessageBox()
{
	setWindowIcon(QIcon(":/rpcs3.ico"));
	setWindowTitle(tr("RPCS3: Fatal Error"));
	setIcon(QMessageBox::Icon::Critical);
	setTextFormat(Qt::TextFormat::RichText);
	setText(QString(R"(
			<p style="white-space: nowrap;">
				%1<br>
				%2<br>
				<a href='https://github.com/RPCS3/rpcs3/wiki/How-to-ask-for-Support'>https://github.com/RPCS3/rpcs3/wiki/How-to-ask-for-Support</a><br>
				%3<br>
			</p>
			)")
		.arg(Qt::convertFromPlainText(QString::fromUtf8(text.data(), text.size())))
		.arg(tr("HOW TO REPORT ERRORS:"))
		.arg(tr("Please, don't send incorrect reports. Thanks for understanding.")));
	layout()->setSizeConstraint(QLayout::SetFixedSize);
}
