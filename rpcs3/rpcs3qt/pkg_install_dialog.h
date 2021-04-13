#pragma once

#include <QDialog>
#include <QListWidget>

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
	std::vector<compat::package_info> GetPathsToInstall() const;

private:
	void MoveItem(int offset) const;

	QListWidget* m_dir_list;
};
