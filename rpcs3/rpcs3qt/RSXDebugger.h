#ifndef RSXDEBUGGER_H
#define RSXDEBUGGER_H

#include "stdafx.h"

#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/GSRender.h"
#include "Emu/RSX/GCM.h"

#include "Utilities/GSL.h"

#include "MemoryViewer.h"

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

class RSXDebugger : public QDialog
{
	Q_OBJECT

	u32 m_addr;

	u32 m_panel_width;
	u32 m_panel_height;
	u32 m_text_width;
	u32 m_text_height;

	u32 pSize;

	QLineEdit* t_addr;
	QPalette* palette_bg;
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

	QWidget* p_buffer_colorA;
	QWidget* p_buffer_colorB;
	QWidget* p_buffer_colorC;
	QWidget* p_buffer_colorD;
	QWidget* p_buffer_depth;
	QWidget* p_buffer_stencil;
	QWidget* p_buffer_tex;

	QImage buffer_img[4];
	QImage depth_img;
	QImage stencil_img;

	QLabel* m_text_transform_program;
	QLabel* m_text_shader_program;

	uint m_cur_texture;

public:
	bool exit;
	RSXDebugger(QWidget* parent);
	~RSXDebugger()
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

public slots:
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void OnChangeToolsAddr();
	virtual void wheelEvent(QWheelEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void OnClickDrawCalls();
	virtual void GoToGet();
	virtual void GoToPut();
	virtual void SetFlags();
	virtual void SetPrograms();
	virtual void OnSelectTexture();
};

#endif // !RSXDEBUGGER_H
