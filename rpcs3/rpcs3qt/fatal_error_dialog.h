#pragma once

#include <QMessageBox>

class fatal_error_dialog : public QMessageBox
{
	Q_OBJECT

public:
	fatal_error_dialog(const std::string& text);
};
