#include "stdafx.h"
#include "vfs_tool_dialog.h"
#include "ui_vfs_tool_dialog.h"
#include "Emu/VFS.h"

vfs_tool_dialog::vfs_tool_dialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::vfs_tool_dialog)
{
	setAttribute(Qt::WA_DeleteOnClose);

	ui->setupUi(this);

	connect(ui->pathEdit, &QLineEdit::textChanged, this, &vfs_tool_dialog::handle_vfs_path);

	handle_vfs_path("");
}

vfs_tool_dialog::~vfs_tool_dialog()
{
}

void vfs_tool_dialog::handle_vfs_path(const QString& path)
{
	const std::string spath = path.toStdString();
	const std::string vfs_get_path = vfs::get(spath);
	const std::string vfs_retrieve_path = vfs::retrieve(spath);
	const std::string vfs_escape_path = vfs::escape(spath);
	const std::string vfs_unescape_path = vfs::unescape(spath);
	const std::string result = fmt::format(
		"Path:\n'%s'\n\n"
		"vfs::get:\n'%s'\n\n"
		"vfs::retrieve:\n'%s'\n\n"
		"vfs::escape:\n'%s'\n\n"
		"vfs::unescape:\n'%s'",
		spath,
		vfs_get_path,
		vfs_retrieve_path,
		vfs_escape_path,
		vfs_unescape_path
	);

	ui->resultEdit->setPlainText(QString::fromStdString(result));
}
