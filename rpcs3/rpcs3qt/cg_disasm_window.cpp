#include "stdafx.h"

#include "cg_disasm_window.h"
#include "Emu/System.h"
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QDialog>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QCoreApplication>
#include <QFontDatabase>
#include "Emu/RSX/CgBinaryProgram.h"

inline QString qstr(const std::string& _in) { return QString::fromUtf8(_in.data(), _in.size()); }
inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }

cg_disasm_window::cg_disasm_window(QWidget* parent): QTabWidget()
{
	setWindowTitle(tr("Cg Disasm"));
	setAttribute(Qt::WA_DeleteOnClose);
	setMinimumSize(200, 150); // seems fine on win 10
	resize(QSize(620, 395));
	
	tab_disasm = new QWidget(this);
	tab_glsl = new QWidget(this);
	addTab(tab_disasm, "ASM");
	addTab(tab_glsl, "GLSL");
	
	QVBoxLayout* layout_disasm = new QVBoxLayout();
	m_disasm_text = new QTextEdit();
	m_disasm_text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_disasm_text->setReadOnly(true);
	m_disasm_text->setWordWrapMode(QTextOption::NoWrap);
	m_disasm_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	layout_disasm->addWidget(m_disasm_text);
	tab_disasm->setLayout(layout_disasm);
	
	QVBoxLayout* layout_glsl = new QVBoxLayout();
	m_glsl_text = new QTextEdit();
	m_glsl_text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_glsl_text->setReadOnly(true);
	m_glsl_text->setWordWrapMode(QTextOption::NoWrap);
	m_glsl_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	layout_glsl->addWidget(m_glsl_text);
	tab_glsl->setLayout(layout_glsl);

	m_disasm_text->setContextMenuPolicy(Qt::CustomContextMenu);
	m_glsl_text->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_disasm_text, &QWidget::customContextMenuRequested, this, &cg_disasm_window::ShowContextMenu);
	connect(m_glsl_text, &QWidget::customContextMenuRequested, this, &cg_disasm_window::ShowContextMenu);
}

void cg_disasm_window::ShowContextMenu(const QPoint &pos) // this is a slot
{
	QMenu myMenu;
	QPoint globalPos = mapToGlobal(pos);
	QAction* clear = new QAction(tr("&Clear"));
	QAction* open = new QAction(tr("Open &Cg binary program"));

	myMenu.addAction(open);
	myMenu.addSeparator();
	myMenu.addAction(clear);

	auto l_clear = [=]() {m_disasm_text->clear(); m_glsl_text->clear();};
	connect(clear, &QAction::triggered, l_clear);
	connect(open, &QAction::triggered, [=] {
		QString filePath = QFileDialog::getOpenFileName(this, tr("Select Cg program object"), m_path_last, tr("Cg program objects (*.fpo;*.vpo);;"));
		if (filePath == NULL) return;
		m_path_last = QFileInfo(filePath).path();
		
		CgBinaryDisasm disasm(sstr(filePath));
		disasm.BuildShaderBody();
		m_disasm_text->setText(qstr(disasm.GetArbShader()));
		m_glsl_text->setText(qstr(disasm.GetGlslShader()));
	});

	myMenu.exec(globalPos);
}
