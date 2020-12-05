#pragma once

#include <QDialog>
#include <QListWidget>

class pkg_install_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit pkg_install_dialog(const QStringList& paths, QWidget* parent = nullptr);
	QStringList GetPathsToInstall() const;

private:
	void MoveItem(int offset);

	QListWidget* m_dir_list;
};
