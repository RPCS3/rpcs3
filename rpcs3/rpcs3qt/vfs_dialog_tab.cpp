#include "vfs_dialog_tab.h"
#include "gui_settings.h"
#include "Utilities/Config.h"

vfs_dialog_tab::vfs_dialog_tab(const QString& name, gui_save list_location, cfg::string* cfg_node, std::shared_ptr<gui_settings> _gui_settings, QWidget* parent)
	: vfs_dialog_path_widget(name, QString::fromStdString(cfg_node->to_string()), QString::fromStdString(cfg_node->def), std::move(list_location), std::move(_gui_settings), parent)
	, m_cfg_node(cfg_node)
{
}

void vfs_dialog_tab::set_settings() const
{
	m_gui_settings->SetValue(m_list_location, get_dir_list());
	m_cfg_node->from_string(get_selected_path());
}
