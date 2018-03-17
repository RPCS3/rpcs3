#pragma once

#include "stdafx.h"
#include "gui_settings.h"
#include "emu_settings.h"
#include "Emu/System.h"

#include "Utilities/File.h"
#include "user_account.h"

#include <QDialog>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QApplication>
#include <QUrl>
#include <QDesktopServices>

class user_manager_dialog : public QDialog
{
	Q_OBJECT
public:
	explicit user_manager_dialog(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<emu_settings> emu_settings, const std::string& dir = "", QWidget* parent = nullptr);
Q_SIGNALS:
	void OnUserLoginSuccess();
private Q_SLOTS:
	void OnUserLogin();
	void OnUserCreate();
	void OnUserRemove();
	void OnUserRename();
	void OnSort(int logicalIndex);
private:
	void Init(const std::string& dir);
	void UpdateTable();
	void GenerateUser(const std::string& username);
	bool ValidateUsername(const QString& textToValidate);

	void ShowContextMenu(const QPoint &pos);

	void closeEvent(QCloseEvent* event) override;

	QTableWidget* m_table;
	std::string m_dir;
	std::string m_selected_user;
	std::vector<UserAccount*> m_user_list;

	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<emu_settings> m_emu_settings;

	QMenu* m_sort_options;

	int m_sort_column;
	bool m_sort_ascending;
};
