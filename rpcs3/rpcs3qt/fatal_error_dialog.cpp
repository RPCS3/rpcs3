#include "fatal_error_dialog.h"

#include <QLayout>
#include <QTextDocument>
#include <QIcon>

const QString document_with_help_text = R"(
<style>
	p {white-space: nowrap;}
</style>
<p>
	%1<br>
	%2<br>
	<a href='https://github.com/RPCS3/rpcs3/wiki/How-to-ask-for-Support'>https://github.com/RPCS3/rpcs3/wiki/How-to-ask-for-Support</a><br>
	%3<br>
</p>
)";

const QString document_without_help_text = R"(
<style>
	p {white-space: nowrap;}
</style>
<p>
	%1<br>
</p>
)";

fatal_error_dialog::fatal_error_dialog(std::string_view text, bool is_html, bool include_help_text) : QMessageBox()
{
	const QString qstr = QString::fromUtf8(text.data(), text.size());
	const QString msg = is_html ? qstr : Qt::convertFromPlainText(qstr);

	QString document_body;
	if (include_help_text) [[likely]]
	{
		document_body = document_with_help_text
			.arg(msg)
			.arg(tr("HOW TO REPORT ERRORS:"))
			.arg(tr("Please, don't send incorrect reports. Thanks for understanding."));
	}
	else
	{
		document_body = document_without_help_text.arg(msg);
	}

#ifndef __APPLE__
	setWindowIcon(QIcon(":/rpcs3.ico"));
#endif
	setWindowTitle(tr("RPCS3: Fatal Error"));
	setIcon(QMessageBox::Icon::Critical);
	setTextFormat(Qt::TextFormat::RichText);
	setText(document_body);
	layout()->setSizeConstraint(QLayout::SetFixedSize);
}
