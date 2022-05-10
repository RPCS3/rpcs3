#pragma once

#include "vfs_dialog_path_widget.h"

#include <memory>

namespace cfg
{
	class string;
}

class gui_settings;

class vfs_dialog_tab : public vfs_dialog_path_widget
{
	Q_OBJECT

public:
	explicit vfs_dialog_tab(const QString& name, gui_save list_location, cfg::string* cfg_node, std::shared_ptr<gui_settings> _gui_settings, QWidget* parent = nullptr);

	void set_settings() const;

private:
	cfg::string* m_cfg_node;
};
