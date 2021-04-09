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

#include <memory>

class gui_settings;

class Buffer : public QGroupBox
{
	Q_OBJECT

	const QSize Panel_Size = QSize(108, 108);
	const QSize Texture_Size = QSize(108, 108);

	u32 m_id;
	bool m_isTex;
	QImage m_image;
	QLabel* m_canvas;
	QSize m_image_size;

public:
	Buffer(bool isTex, u32 id, const QString& name, QWidget* parent = nullptr);
	void showImage(const QImage& image = QImage());
	void ShowWindowed() const;
};

class rsx_debugger : public QDialog
{
	Q_OBJECT

	u32 m_addr = 0;

	QLineEdit* m_addr_line;

	QTableWidget* m_list_commands;
	QTableWidget* m_list_captured_frame;
	QTableWidget* m_list_captured_draw_calls;
	QListWidget* m_list_index_buffer;
	QTabWidget* m_tw_rsx;

	Buffer* m_buffer_colorA;
	Buffer* m_buffer_colorB;
	Buffer* m_buffer_colorC;
	Buffer* m_buffer_colorD;
	Buffer* m_buffer_depth;
	Buffer* m_buffer_stencil;
	Buffer* m_buffer_tex;

	QLabel* m_text_transform_program;
	QLabel* m_text_shader_program;

	uint m_cur_texture = 0;

	std::shared_ptr<gui_settings> m_gui_settings;

public:
	explicit rsx_debugger(std::shared_ptr<gui_settings> gui_settings, QWidget* parent = nullptr);
	~rsx_debugger() = default;

	void UpdateInformation() const;
	void GetMemory() const;
	void GetBuffers() const;

	QString DisAsmCommand(u32 cmd, u32 count, u32 ioAddr) const;

	void SetPC(const uint pc);

public Q_SLOTS:
	virtual void OnClickDrawCalls();

protected:
	void closeEvent(QCloseEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	bool eventFilter(QObject* object, QEvent* event) override;

private:
	void PerformJump(u32 address);
};
