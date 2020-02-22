#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QFont>
#include <QFontDatabase>

class memory_viewer_panel : public QDialog
{
	u32 m_addr;
	u32 m_colcount;
	u32 m_rowcount;

	QLineEdit* m_addr_line;

	QLabel* m_mem_addr;
	QLabel* m_mem_hex;
	QLabel* m_mem_ascii;

	QFontMetrics* m_fontMetrics;

public:
	bool exit;
	memory_viewer_panel(QWidget* parent);
	~memory_viewer_panel();

	virtual void wheelEvent(QWheelEvent *event);

	virtual void ShowMemory();
	void SetPC(const uint pc);

	//Static methods
	static void ShowImage(QWidget* parent, u32 addr, int mode, u32 sizex, u32 sizey, bool flipv);
};
