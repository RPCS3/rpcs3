#pragma once

#include <optional>
#include <QDialog>
#include <QComboBox>

class clans_settings_dialog : public QDialog
{
	Q_OBJECT
public:
	clans_settings_dialog(QWidget* parent = nullptr);

private:
	void refresh_combobox();

private:
	QComboBox* m_cbx_servers = nullptr;
	QComboBox* m_cbx_protocol = nullptr;
};

class clans_add_server_dialog : public QDialog
{
	Q_OBJECT
public:
	clans_add_server_dialog(QWidget* parent = nullptr);
	const std::optional<std::pair<std::string, std::string>>& get_new_server() const;

private:
	std::optional<std::pair<std::string, std::string>> m_new_server;
};
