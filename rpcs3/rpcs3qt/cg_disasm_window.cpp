#include "cg_disasm_window.h"
#include "gui_settings.h"
#include "syntax_highlighter.h"

#include <QSplitter>
#include <QMenu>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QMimeData>

#include "Emu/RSX/Program/CgBinaryProgram.h"

LOG_CHANNEL(gui_log, "GUI");

cg_disasm_window::cg_disasm_window(std::shared_ptr<gui_settings> gui_settings)
	: m_gui_settings(std::move(gui_settings))
{
	setWindowTitle(tr("Cg Disasm"));
	setObjectName("cg_disasm");
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_StyledBackground);
	setAcceptDrops(true);
	setMinimumSize(QSize(200, 150)); // seems fine on win 10
	resize(QSize(620, 395));

	m_path_last = m_gui_settings->GetValue(gui::fd_cg_disasm).toString();

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
	QMenu menu;
	QAction* clear = new QAction(tr("&Clear"));
	QAction* open = new QAction(tr("Open &Cg binary program"));

	menu.addAction(open);
	menu.addSeparator();
	menu.addAction(clear);

	connect(clear, &QAction::triggered, [this]()
	{
		m_disasm_text->clear();
		m_glsl_text->clear();
	});

	connect(open, &QAction::triggered, [this]()
	{
		const QString file_path = QFileDialog::getOpenFileName(this, tr("Select Cg program object"), m_path_last, tr("Cg program objects (*.fpo;*.vpo);;"));
		if (file_path.isEmpty())
			return;
		m_path_last = file_path;
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

	menu.exec(origin);
}

void cg_disasm_window::ShowDisasm() const
{
	if (QFileInfo(m_path_last).isFile())
	{
		CgBinaryDisasm disasm(m_path_last.toStdString());
		disasm.BuildShaderBody();
		m_disasm_text->setText(QString::fromStdString(disasm.GetArbShader()));
		m_glsl_text->setText(QString::fromStdString(disasm.GetGlslShader()));
		m_gui_settings->SetValue(gui::fd_cg_disasm, m_path_last);
	}
	else if (!m_path_last.isEmpty())
	{
		gui_log.error("CgDisasm: Failed to open %s", m_path_last);
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
