#pragma once

#include "Emu/config_mode.h"

#include <QDialog>
#include <QLabel>
#include <QTextEdit>

class config_checker : public QDialog
{
	Q_OBJECT

public:
	enum class checker_mode
	{
		config,
		log,
		gamelist
	};

	config_checker(QWidget* parent, const QString& content_or_serial, checker_mode mode, const std::string& db_config = {});

private:
	void check_config(cfg_mode mode);
	bool check_config(cfg_mode mode, QString content_or_serial, QString& result);

	QLabel* m_label = nullptr;
	QTextEdit* m_text_box = nullptr;

	checker_mode m_checker_mode = checker_mode::config;
	QString m_content_or_serial;
	std::string m_db_config;
	std::string m_serial;
};
