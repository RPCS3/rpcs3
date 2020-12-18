#pragma once

#include "stdafx.h"

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QFont>
#include <QFontDatabase>

class memory_viewer_panel : public QDialog
{
	Q_OBJECT

public:
	memory_viewer_panel(QWidget* parent, u32 addr = 0);
	~memory_viewer_panel();

	enum class color_format : int
	{
		RGB,
		ARGB,
		RGBA,
		ABGR
	};
	Q_ENUM(color_format)

	bool exit;

protected:
	void wheelEvent(QWheelEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	u32 m_addr;
	u32 m_colcount;
	u32 m_rowcount;

	QLineEdit* m_addr_line;

	QLabel* m_mem_addr;
	QLabel* m_mem_hex;
	QLabel* m_mem_ascii;

	QFontMetrics* m_fontMetrics;

	std::string getHeaderAtAddr(u32 addr);
	void scroll(s32 steps);
	void SetPC(const uint pc);

	virtual void ShowMemory();

	static void ShowImage(QWidget* parent, u32 addr, color_format format, u32 sizex, u32 sizey, bool flipv);
};
