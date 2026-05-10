#pragma once

#include <QDialog>

namespace Ui
{
	class vfs_tool_dialog;
}

class vfs_tool_dialog : public QDialog
{
	Q_OBJECT

public:
	vfs_tool_dialog(QWidget *parent);
	virtual ~vfs_tool_dialog();

private:
	std::unique_ptr<Ui::vfs_tool_dialog> ui;

private Q_SLOTS:
	void handle_vfs_path(const QString& path);
};
