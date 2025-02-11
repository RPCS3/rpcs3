#pragma once

#include <QTextEdit>
#include <QDropEvent>

#include <memory>

class AsmHighlighter;
class GlslHighlighter;
class gui_settings;

class cg_disasm_window : public QWidget
{
	Q_OBJECT

private Q_SLOTS:
	void ShowContextMenu(const QPoint &pos);

private:
	void ShowDisasm() const;
	bool IsValidFile(const QMimeData& md, bool save = false);

	QString m_path_last;
	QTextEdit* m_disasm_text;
	QTextEdit* m_glsl_text;

	std::shared_ptr<gui_settings> m_gui_settings;

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
