#pragma once

#include "emu_settings.h"
#include "Utilities/stereo_config.h"

#include <QDialog>
#include <QTableWidget>
#include <QVector3D>

class color_wedge_widget : public QWidget
{
public:
	color_wedge_widget(QWidget* parent = nullptr);
	virtual ~color_wedge_widget();

	void set_option(stereo_render_mode_options option);
	void set_custom_matrices(const stereo_config::stereo_matrices& matrices);

	const stereo_config& get_stereo_config() const { return m_stereo_config; }

protected:
	QVector3D apply_matrix(const QVector3D& left, const QVector3D& right, const stereo_config::stereo_matrices& m);

	QVector3D apply_anaglyph_matrix(const QVector3D& left, const QVector3D& right);

	void paintEvent(QPaintEvent* event) override;

	stereo_config m_stereo_config = stereo_config(false);
	QImage m_img;
};

class anaglyph_settings_dialog : public QDialog
{
public:
	anaglyph_settings_dialog(QWidget* parent, std::shared_ptr<emu_settings> emu_settings);

private:
	std::shared_ptr<emu_settings> m_emu_settings;
	color_wedge_widget* m_widget_reference = nullptr;
	color_wedge_widget* m_widget = nullptr;
	QTableWidget* m_matrix_left = nullptr;
	QTableWidget* m_matrix_right = nullptr;
	bool m_updating = false;

	QTableWidget* create_matrix_table();

	void read_matrix(mat3f& matrix, QTableWidget* table) const;

	void update_matrix(QTableWidget* table, const mat3f& matrix, bool is_custom);

	void update_matrices();

	void apply_custom_matrix();
};
