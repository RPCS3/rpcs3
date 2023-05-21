#pragma once

#include <QMessageBox>

#include <string_view>

class fatal_error_dialog : public QMessageBox
{
	Q_OBJECT

public:
	explicit fatal_error_dialog(std::string_view text, bool is_html, bool include_help_text);
};
