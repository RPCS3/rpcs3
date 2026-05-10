#pragma once

#include "util/types.hpp"

#include <QDialog>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QEvent>
#include <QListWidget>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextEdit>

#include <memory>

class gui_settings;
class QPoint;

class Buffer : public QGroupBox
{
	Q_OBJECT

	const QSize Panel_Size = QSize(320, 180);
	const QSize Texture_Size = QSize(320, 180);

	u32 m_id;
	bool m_isTex;
	QImage m_image;
	QLabel* m_canvas;
	QSize m_image_size;
	u32 m_window_counter = 1;

public:
	std::vector<u8> cache;

	Buffer(bool isTex, u32 id, const QString& name, QWidget* parent = nullptr);
	void showImage(QImage&& image);
	void ShowWindowed();

	void ShowContextMenu(const QPoint& pos);
};

class rsx_debugger : public QDialog
{
	Q_OBJECT

	QTableWidget* m_list_captured_frame;
	QTableWidget* m_list_captured_draw_calls;
	QListWidget* m_list_index_buffer;
	QTabWidget* m_tw_rsx;

	const QString enabled_textures_text = tr(" Enabled Textures Indices: ");

	Buffer* m_buffer_colorA;
	Buffer* m_buffer_colorB;
	Buffer* m_buffer_colorC;
	Buffer* m_buffer_colorD;
	Buffer* m_buffer_depth;
	Buffer* m_buffer_stencil;
	Buffer* m_buffer_tex;
	QLabel* m_enabled_textures_label;

	QTextEdit* m_transform_disasm;
	QTextEdit* m_fragment_disasm;

	u32 m_cur_texture = 0;
	u32 m_texture_format_override = 0;

	std::shared_ptr<gui_settings> m_gui_settings;

public:
	explicit rsx_debugger(std::shared_ptr<gui_settings> gui_settings, QWidget* parent = nullptr);
	~rsx_debugger() = default;

	void UpdateInformation() const;
	void GetMemory() const;
	void GetBuffers() const;

public Q_SLOTS:
	virtual void OnClickDrawCalls();

protected:
	void closeEvent(QCloseEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	bool eventFilter(QObject* object, QEvent* event) override;

private:
	void PerformJump(u32 address);

	void GetVertexProgram() const;
	void GetFragmentProgram() const;
};
