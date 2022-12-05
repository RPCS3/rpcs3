#include "fatal_error_dialog.h"

#include <QLayout>
#include <QTextDocument>
#include <QIcon>

static QString process_dialog_text(std::string_view text)
{
	auto html = Qt::convertFromPlainText(QString::fromUtf8(text.data(), text.size()));

	// Let's preserve some html elements destroyed by convertFromPlainText
	const QRegExp link_re{ R"(&lt;a\shref='([a-z0-9?=&#:\/\.\-]+)'&gt;([a-z0-9?=&#:\/\.\-]+)&lt;\/a&gt;)", Qt::CaseSensitive, QRegExp::RegExp2};
	html = html.replace(link_re, "<a href=\\1>\\2</a>");

	return html;
}

fatal_error_dialog::fatal_error_dialog(std::string_view text) : QMessageBox()
{
#ifndef __APPLE__
	setWindowIcon(QIcon(":/rpcs3.ico"));
#endif
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
		.arg(process_dialog_text(text))
		.arg(tr("HOW TO REPORT ERRORS:"))
		.arg(tr("Please, don't send incorrect reports. Thanks for understanding.")));
	layout()->setSizeConstraint(QLayout::SetFixedSize);
}
