#ifndef MEMORYVIEWER_H
#define MEMORYVIEWER_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QGroupBox>
#include <QFontDatabase>
#include <QLabel>
#include <QFont>
#include <QScrollBar>
#include <QPainter>
#include <QMouseEvent>

class memory_viewer_panel : public QDialog
{
	u32 m_addr;
	u32 m_colcount;
	u32 m_rowcount;
	u32 pSize;

	QLineEdit* t_addr;
	QSpinBox* sb_bytes;

	QSpinBox* sb_img_size_x;
	QSpinBox* sb_img_size_y;
	QComboBox* cbox_img_mode;

	QLabel* t_mem_addr;
	QLabel* t_mem_hex;
	QLabel* t_mem_ascii;
	QFont mono;
	QFontMetrics* fontMetrics;
	QSize textSize;
	QPalette pal_bg;
	QPalette pal_fg;

public:
	bool exit;
	memory_viewer_panel(QWidget* parent);
	~memory_viewer_panel()
	{
		exit = true;
	}

	virtual void wheelEvent(QWheelEvent *event);

	virtual void ShowMemory();
	void SetPC(const uint pc) { m_addr = pc; }

	//Static methods
	static void ShowImage(QWidget* parent, u32 addr, int mode, u32 sizex, u32 sizey, bool flipv);
};

#endif // !MEMORYVIEWER_H
