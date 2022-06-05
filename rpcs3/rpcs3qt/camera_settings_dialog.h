#pragma once

#include <QCamera>
#include <QDialog>

namespace Ui
{
	class camera_settings_dialog;
}

class camera_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	camera_settings_dialog(QWidget* parent = nullptr);
	virtual ~camera_settings_dialog();

private Q_SLOTS:
	void handle_camera_change(int index);
	void handle_settings_change(int index);

private:
	void load_config();
	void save_config();

	std::unique_ptr<Ui::camera_settings_dialog> ui;
	std::shared_ptr<QCamera> m_camera;
};
