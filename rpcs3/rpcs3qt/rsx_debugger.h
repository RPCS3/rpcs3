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

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QTabWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QPalette>
#include <QFont>
#include <QSignalMapper>
#include <QPixmap>

class Buffer : public QWidget
{
	QHBoxLayout* m_layout;
	QImage m_scaled;
	u32 m_id;
	bool m_isTex;

public:
	QLabel* m_canvas;
	QImage m_image;

	Buffer(QWidget* parent, bool isTex, u32 id = 4)
		: QWidget(parent), m_isTex(isTex), m_id(id){};
	void showImage(const QImage& image = QImage());

private:
	void mouseDoubleClickEvent(QMouseEvent* event);
};

class rsx_debugger : public QDialog
{
	Q_OBJECT

	u32 m_addr;

	u32 m_panel_width;
	u32 m_panel_height;
	u32 m_text_width;
	u32 m_text_height;

	u32 pSize;

	QLineEdit* t_addr;
	QPalette palette_bg;
	QFont mono;
	QFontMetrics* fontMetrics;

	u32 m_item_count;
	QTableWidget* m_list_commands;
	QTableWidget* m_list_captured_frame;
	QTableWidget* m_list_captured_draw_calls;
	QTableWidget* m_list_flags;
	QTableWidget* m_list_lightning;
	QTableWidget* m_list_texture;
	QTableWidget* m_list_settings;
	QListWidget* m_list_index_buffer;

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

public:
	bool exit;
	rsx_debugger(QWidget* parent);
	~rsx_debugger()
	{
		exit = true;
	}

	virtual void UpdateInformation();
	virtual void GetMemory();
	virtual void GetBuffers();
	virtual void GetFlags();
	virtual void GetLightning();
	virtual void GetTexture();
	virtual void GetSettings();

	const char* ParseGCMEnum(u32 value, u32 type);
	QString DisAsmCommand(u32 cmd, u32 count, u32 currentAddr, u32 ioAddr);

	void SetPC(const uint pc) { m_addr = pc; }

private:
	QSignalMapper *signalMapper;

public Q_SLOTS:
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void wheelEvent(QWheelEvent* event);
	virtual void OnClickDrawCalls();
	virtual void SetFlags();
	virtual void SetPrograms();
};
