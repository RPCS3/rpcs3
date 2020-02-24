#pragma once

#include "stdafx.h"
#include "user_account.h"

#include <QDialog>
#include <QTableWidget>

class gui_settings;

class user_manager_dialog : public QDialog
{
	Q_OBJECT
public:
	explicit user_manager_dialog(std::shared_ptr<gui_settings> gui_settings, QWidget* parent = nullptr);
Q_SIGNALS:
	void OnUserLoginSuccess();
private Q_SLOTS:
	void OnUserLogin();
	void OnUserCreate();
	void OnUserRemove();
	void OnUserRename();
	void OnSort(int logicalIndex);
private:
	void Init();
	void UpdateTable(bool mark_only = false);
	void GenerateUser(const std::string& user_id, const std::string& username);
	bool ValidateUsername(const QString& text_to_validate);

	void ShowContextMenu(const QPoint &pos);

	void closeEvent(QCloseEvent* event) override;
	bool eventFilter(QObject* object, QEvent* event) override;

	u32 GetUserKey();

	QTableWidget* m_table;
	std::string m_active_user;
	std::map<u32, UserAccount> m_user_list;

	std::shared_ptr<gui_settings> m_gui_settings;

	int m_sort_column = 1;
	bool m_sort_ascending = true;
};
