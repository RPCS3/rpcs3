#pragma once

#include <QTextEdit>
#include <QDropEvent>

#include "stdafx.h"

class AsmHighlighter;
class GlslHighlighter;
class gui_settings;

class cg_disasm_window : public QWidget
{
	Q_OBJECT

private Q_SLOTS:
	void ShowContextMenu(const QPoint &pos);
	void ShowDisasm();
	bool IsValidFile(const QMimeData& md, bool save = false);

private:
	QString m_path_last;
	QTextEdit* m_disasm_text;
	QTextEdit* m_glsl_text;
	QList<QUrl> m_urls;

	QAction *openCgBinaryProgram;

	std::shared_ptr<gui_settings> xgui_settings;

	AsmHighlighter* sh_asm;
	GlslHighlighter* sh_glsl;

public:
	explicit cg_disasm_window(std::shared_ptr<gui_settings> xSettings);

protected:
	void dropEvent(QDropEvent* ev) override;
	void dragEnterEvent(QDragEnterEvent* ev) override;
	void dragMoveEvent(QDragMoveEvent* ev) override;
	void dragLeaveEvent(QDragLeaveEvent* ev) override;
};
