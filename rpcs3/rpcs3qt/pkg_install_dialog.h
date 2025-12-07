#pragma once

#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QDialogButtonBox>

namespace compat
{
	struct package_info;
}

class game_compatibility;

class pkg_install_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit pkg_install_dialog(const QStringList& paths, game_compatibility* compat, QWidget* parent = nullptr);
	std::vector<compat::package_info> get_paths_to_install() const;
	bool precompile_caches() const { return m_precompile_caches; }
	bool create_desktop_shortcuts() const { return m_create_desktop_shortcuts; }
	bool create_app_shortcut() const { return m_create_app_shortcut; }

private:
	void update_info(QLabel* installation_info, QDialogButtonBox* buttons) const;
	void move_item(int offset) const;

	QListWidget* m_dir_list = nullptr;
	bool m_precompile_caches = false;
	bool m_create_desktop_shortcuts = false;
	bool m_create_app_shortcut = false;
};
