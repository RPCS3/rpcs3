#pragma once

#include "util/types.hpp"
#include "user_account.h"

#include <QDialog>
#include <QTableWidget>
#include <string>
#include <memory>
#include <map>

class gui_settings;
class persistent_settings;

class user_manager_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit user_manager_dialog(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<persistent_settings> persistent_settings, QWidget* parent = nullptr);

Q_SIGNALS:
	void OnUserLoginSuccess();

private Q_SLOTS:
	void OnUserLogin();
	void OnUserCreate();
	void OnUserRemove();
	void OnUserRename();
	void OnSort(int logicalIndex);

protected:
	void mouseDoubleClickEvent(QMouseEvent* ev) override;

private:
	void Init();
	void UpdateTable(bool mark_only = false);
	static void GenerateUser(const std::string& user_id, const std::string& username);
	static bool ValidateUsername(const QString& text_to_validate);

	void ShowContextMenu(const QPoint &pos);

	void closeEvent(QCloseEvent* event) override;
	bool eventFilter(QObject* object, QEvent* event) override;

	u32 GetUserKey() const;

	QTableWidget* m_table = nullptr;
	std::string m_active_user;
	std::map<u32, user_account> m_user_list;

	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<persistent_settings> m_persistent_settings;

	int m_sort_column = 1;
	bool m_sort_ascending = true;
};
