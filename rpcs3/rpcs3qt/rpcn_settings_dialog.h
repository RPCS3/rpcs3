#pragma once

#include <QDialog>
#include <QLineEdit>

class rpcn_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	rpcn_settings_dialog(QWidget* parent = nullptr);

	bool save_config();
	bool create_account();

protected:
	QLineEdit *m_edit_host, *m_edit_npid, *m_edit_token;

	std::string generate_npid();
};
