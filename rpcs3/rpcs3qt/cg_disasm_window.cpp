#include "stdafx.h"

#include "cg_disasm_window.h"

#include <QSplitter>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QDialog>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QCoreApplication>
#include <QFontDatabase>
#include <QMimeData>

#include "Emu/RSX/CgBinaryProgram.h"

LOG_CHANNEL(gui_log, "GUI");

constexpr auto qstr = QString::fromStdString;
inline std::string sstr(const QString& _in) { return _in.toStdString(); }

cg_disasm_window::cg_disasm_window(std::shared_ptr<gui_settings> xSettings): xgui_settings(xSettings)
{
	setWindowTitle(tr("Cg Disasm"));
	setObjectName("cg_disasm");
	setAttribute(Qt::WA_DeleteOnClose);
	setAcceptDrops(true);
	setMinimumSize(QSize(200, 150)); // seems fine on win 10
	resize(QSize(620, 395));

	m_path_last = xgui_settings->GetValue(gui::fd_cg_disasm).toString();

	m_disasm_text = new QTextEdit(this);
	m_disasm_text->setReadOnly(true);
	m_disasm_text->setWordWrapMode(QTextOption::NoWrap);
	m_disasm_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	m_glsl_text = new QTextEdit(this);
	m_glsl_text->setReadOnly(true);
	m_glsl_text->setWordWrapMode(QTextOption::NoWrap);
	m_glsl_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	// m_disasm_text syntax highlighter
	sh_asm = new AsmHighlighter(m_disasm_text->document());

	// m_glsl_text syntax highlighter
	sh_glsl = new GlslHighlighter(m_glsl_text->document());

	QSplitter* splitter = new QSplitter();
	splitter->addWidget(m_disasm_text);
	splitter->addWidget(m_glsl_text);

	QHBoxLayout* layout = new QHBoxLayout();
	layout->addWidget(splitter);

	setLayout(layout);

	m_disasm_text->setContextMenuPolicy(Qt::CustomContextMenu);
	m_glsl_text->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_disasm_text, &QWidget::customContextMenuRequested, this, &cg_disasm_window::ShowContextMenu);
	connect(m_glsl_text, &QWidget::customContextMenuRequested, this, &cg_disasm_window::ShowContextMenu);

	ShowDisasm();
}

void cg_disasm_window::ShowContextMenu(const QPoint &pos)
{
	QMenu myMenu;
	QAction* clear = new QAction(tr("&Clear"));
	QAction* open = new QAction(tr("Open &Cg binary program"));

	myMenu.addAction(open);
	myMenu.addSeparator();
	myMenu.addAction(clear);

	connect(clear, &QAction::triggered, [=]
	{
		m_disasm_text->clear();
		m_glsl_text->clear();
	});

	connect(open, &QAction::triggered, [=]
	{
		QString filePath = QFileDialog::getOpenFileName(this, tr("Select Cg program object"), m_path_last, tr("Cg program objects (*.fpo;*.vpo);;"));
		if (filePath == NULL) return;
		m_path_last = filePath;
		ShowDisasm();
	});

	const auto obj = qobject_cast<QTextEdit*>(sender());

	QPoint origin;

	if (obj == m_disasm_text)
	{
		origin = m_disasm_text->viewport()->mapToGlobal(pos);
	}
	else if (obj == m_glsl_text)
	{
		origin = m_glsl_text->viewport()->mapToGlobal(pos);
	}
	else
	{
		origin = mapToGlobal(pos);
	}

	myMenu.exec(origin);
}

void cg_disasm_window::ShowDisasm()
{
	if (QFileInfo(m_path_last).isFile())
	{
		CgBinaryDisasm disasm(sstr(m_path_last));
		disasm.BuildShaderBody();
		m_disasm_text->setText(qstr(disasm.GetArbShader()));
		m_glsl_text->setText(qstr(disasm.GetGlslShader()));
		xgui_settings->SetValue(gui::fd_cg_disasm, m_path_last);
	}
	else if (!m_path_last.isEmpty())
	{
		gui_log.error("CgDisasm: Failed to open %s", sstr(m_path_last));
	}
}

bool cg_disasm_window::IsValidFile(const QMimeData& md, bool save)
{
	const QList<QUrl> urls = md.urls();

	if (urls.count() > 1)
	{
		return false;
	}

	const QString suff = QFileInfo(urls[0].fileName()).suffix().toLower();

	if (suff == "fpo" || suff == "vpo")
	{
		if (save)
		{
			m_path_last = urls[0].toLocalFile();
		}
		return true;
	}
	return false;
}

void cg_disasm_window::dropEvent(QDropEvent* ev)
{
	if (IsValidFile(*ev->mimeData(), true))
	{
		ShowDisasm();
	}
}

void cg_disasm_window::dragEnterEvent(QDragEnterEvent* ev)
{
	if (IsValidFile(*ev->mimeData()))
	{
		ev->accept();
	}
}

void cg_disasm_window::dragMoveEvent(QDragMoveEvent* ev)
{
	if (IsValidFile(*ev->mimeData()))
	{
		ev->accept();
	}
}

void cg_disasm_window::dragLeaveEvent(QDragLeaveEvent* ev)
{
	ev->accept();
}
