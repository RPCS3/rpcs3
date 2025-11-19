#pragma once

#include <QTableWidget>
#include <QLabel>

#include <memory>

class gui_settings;

namespace cfg
{
	class device_entry;
}

class vfs_dialog_usb_tab : public QWidget
{
	Q_OBJECT

public:
	explicit vfs_dialog_usb_tab(cfg::device_entry* cfg_node, std::shared_ptr<gui_settings> _gui_settings, QWidget* parent = nullptr);

	void set_settings() const;

	// Reset this tab without saving the settings yet
	void reset() const;

protected:
	void mouseDoubleClickEvent(QMouseEvent* ev) override;

private:
	void show_usb_input_dialog(int index);
	void show_context_menu(const QPoint& pos);
	void double_clicked_slot(QTableWidgetItem* item);

	cfg::device_entry* m_cfg_node;
	std::shared_ptr<gui_settings> m_gui_settings;
	QTableWidget* m_usb_table;
};
