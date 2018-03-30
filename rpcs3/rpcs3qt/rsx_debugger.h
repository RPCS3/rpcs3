#pragma once

#include "stdafx.h"

#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/GSRender.h"
#include "Emu/RSX/GCM.h"

#include "Utilities/GSL.h"

#include "memory_viewer_panel.h"
#include "table_item_delegate.h"
#include "gui_settings.h"

#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QEvent>
#include <QTabWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QFont>
#include <QSignalMapper>
#include <QPixmap>

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
	Buffer(bool isTex, u32 id, const QString& name, QWidget* parent = 0);
	void showImage(const QImage& image = QImage());
	void ShowWindowed();
};

class rsx_debugger : public QDialog
{
	Q_OBJECT

	u32 m_addr;

	QLineEdit* m_addr_line;

	QTableWidget* m_list_commands;
	QTableWidget* m_list_captured_frame;
	QTableWidget* m_list_captured_draw_calls;
	QTableWidget* m_list_flags;
	QTableWidget* m_list_lightning;
	QTableWidget* m_list_texture;
	QTableWidget* m_list_settings;
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

	uint m_cur_texture;

	std::shared_ptr<gui_settings> m_gui_settings;

public:
	bool exit;
	rsx_debugger(std::shared_ptr<gui_settings> gui_settings, QWidget* parent = 0);
	~rsx_debugger();

	virtual void UpdateInformation();
	virtual void GetMemory();
	virtual void GetBuffers();
	virtual void GetFlags();
	virtual void GetLightning();
	virtual void GetTexture();
	virtual void GetSettings();

	const char* ParseGCMEnum(u32 value, u32 type);
	QString DisAsmCommand(u32 cmd, u32 count, u32 currentAddr, u32 ioAddr);

	void SetPC(const uint pc);

public Q_SLOTS:
	virtual void OnClickDrawCalls();
	virtual void SetFlags();
	virtual void SetPrograms();

protected:
	virtual void closeEvent(QCloseEvent* event) override;
	virtual void keyPressEvent(QKeyEvent* event) override;
	virtual bool eventFilter(QObject* object, QEvent* event) override;

private:
	void PerformJump(u32 address);
};
