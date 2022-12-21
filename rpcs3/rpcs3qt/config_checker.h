#pragma once

#include <QDialog>

class config_checker : public QDialog
{
	Q_OBJECT

public:
	config_checker(QWidget* parent, const QString& path, bool is_log);

	bool check_config(QString content, QString& result, bool is_log);
};
