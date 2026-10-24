#pragma once

#include <QDialog>
#include <QFutureWatcher>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>

#include <string>
#include <vector>

class avatar_selection_dialog : public QDialog
{
	Q_OBJECT

public:
	avatar_selection_dialog(const std::string& user_id, QWidget* parent = nullptr);
	~avatar_selection_dialog();

	static QPixmap get_user_avatar(const std::string& user_id, int size);

private:
	void load_avatars();
	void update_current_avatar();
	void update_status();
	void select_avatar(const std::string& path);
	void browse_avatar();

	enum item_role
	{
		path_role = Qt::UserRole,
		usable_role,
	};

	std::string m_user_id;
	std::vector<std::string> m_paths;
	QFutureWatcher<QImage> m_watcher;
	int m_loaded_count = 0;
	int m_unusable_count = 0;

	QLabel* m_current_icon = nullptr;
	QLabel* m_current_label = nullptr;
	QLabel* m_status_label = nullptr;
	QListWidget* m_list = nullptr;
	QPushButton* m_select_button = nullptr;
};
