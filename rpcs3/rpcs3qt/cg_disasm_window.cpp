#include "stdafx.h"

#include "cg_disasm_window.h"
#include "syntax_highlighter.h"

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
	syntax_highlighter* sh_asm = new syntax_highlighter(m_disasm_text->document());
	sh_asm->AddCommentRule("#",    QColor(Qt::darkGreen));
	sh_asm->AddCommentRule("\/\*", QColor(Qt::darkGreen), true, "\*\/");
	sh_asm->AddSimpleRule(QStringList("^([^\\s]+).*$"),                 QColor(Qt::darkBlue));    // Instructions
	sh_asm->AddSimpleRule(QStringList(",?\\s(-?R\\d[^,;\\s]*)"),        QColor(Qt::darkRed));     // -R0.*
	sh_asm->AddSimpleRule(QStringList(",?\\s(-?H\\d[^,;\\s]*)"),        QColor(Qt::red));         // -H1.*
	sh_asm->AddSimpleRule(QStringList(",?\\s(-?v\\[\\d\\]*[^,;\\s]*)"), QColor(Qt::darkCyan));    // -v[xyz].*
	sh_asm->AddSimpleRule(QStringList(",?\\s(-?o\\[\\d\\]*[^,;\\s]*)"), QColor(Qt::darkMagenta)); // -o[xyz].*
	sh_asm->AddSimpleRule(QStringList(",?\\s(-?c\\[\\d\\]*[^,;\\s]*)"), QColor(Qt::darkYellow));  // -c[xyz].*
	//sh_asm->AddMultiRule(
	//	"^([^\\s]+)(?:,?\\s*([^,\\;\\s]+))?(?:,?\\s*([^,\\;\\s]+))?(?:,?\\s*([^,\\;\\s]+))?(?:,?\\s*([^,\\;\\s]+))?.*$",
	//	{ QColor(Qt::black), QColor(Qt::darkBlue), QColor(Qt::darkRed), QColor(Qt::darkMagenta), QColor(Qt::darkYellow), QColor(Qt::darkCyan) });

	// m_glsl_text syntax highlighter
	QStringList glsl_syntax = QStringList()
		// Selection-Iteration-Jump Statements:
		<< "if"     << "else"  << "switch"   << "case"    << "default"
		<< "for"    << "while" << "do"       << "foreach" //?
		<< "return" << "break" << "continue" << "discard"
		// Misc:
		<< "void"    << "char"     << "short"     << "long"      << "template"
		<< "class"   << "struct"   << "union"     << "enum"
		<< "static"  << "virtual"  << "inline"    << "explicit"
		<< "public"  << "private"  << "protected" << "namespace"
		<< "typedef" << "typename" << "signed"    << "unsigned"
		<< "friend"  << "operator" << "signals"   << "slots"
		// Qualifiers:
		<< "in"     << "packed" << "precision" << "const"     << "smooth"        << "sample"
		<< "out"    << "shared" << "highp"     << "invariant" << "noperspective" << "centroid"
		<< "inout"  << "std140" << "mediump"   << "uniform"   << "flat"          << "patch"
		<< "buffer" << "std430" << "lowp"      
		<< "image"
		// Removed Qualifiers?
		<< "attribute" << "varying"
		// Memory Qualifiers
		<< "coherent" << "volatile" << "restrict" << "readonly" << "writeonly"
		// Layout Qualifiers:
		//<< "subroutine" << "layout"     << "xfb_buffer" << "textureGrad"  << "texture" << "dFdx" << "dFdy"
		//<< "location"   << "component"  << "binding"    << "offset"
		//<< "xfb_offset" << "xfb_stride" << "vertices"   << "max_vertices"
		// Scalars and Vectors:
		<< "bool"  << "int"   << "uint"  << "float" << "double"
		<< "bvec2" << "ivec2" << "uvec2" << "vec2"  << "dvec2"
		<< "bvec3" << "ivec3" << "uvec3" << "vec3"  << "dvec3"
		<< "bvec4" << "ivec4" << "uvec4" << "vec4"  << "dvec4"
		// Matrices:
		<< "mat2" << "mat2x3" << "mat2x4"
		<< "mat3" << "mat3x2" << "mat3x4"
		<< "mat4" << "mat4x2" << "mat4x3"
		// Sampler Types:
		<< "sampler1D"        << "isampler1D"        << "usampler1D"
		<< "sampler2D"        << "isampler2D"        << "usampler2D"
		<< "sampler3D"        << "isampler3D"        << "usampler3D"
		<< "samplerCube"      << "isamplerCube"      << "usamplerCube"
		<< "sampler2DRect"    << "isampler2DRect"    << "usampler2DRect"
		<< "sampler1DArray"   << "isampler1DArray"   << "usampler1DArray"
		<< "sampler2DArray"   << "isampler2DArray"   << "usampler2DArray"
		<< "samplerCubeArray" << "isamplerCubeArray" << "usamplerCubeArray"
		<< "samplerBuffer"    << "isamplerBuffer"    << "usamplerBuffer"
		<< "sampler2DMS"      << "isampler2DMS"      << "usampler2DMS"
		<< "sampler2DMSArray" << "isampler2DMSArray" << "usampler2DMSArray"
		// Shadow Samplers:
		<< "sampler1DShadow"
		<< "sampler2DShadow"
		<< "samplerCubeShadow"
		<< "sampler2DRectShadow"
		<< "sampler1DArrayShadow"
		<< "sampler2DArrayShadow"
		<< "samplerCubeArrayShadow"
		// Image Types:
		<< "image1D"        << "iimage1D"        << "uimage1D"
		<< "image2D"        << "iimage2D"        << "uimage2D"
		<< "image3D"        << "iimage3D"        << "uimage3D"
		<< "imageCube"      << "iimageCube"      << "uimageCube"
		<< "image2DRect"    << "iimage2DRect"    << "uimage2DRect"
		<< "image1DArray"   << "iimage1DArray"   << "uimage1DArray"
		<< "image2DArray"   << "iimage2DArray"   << "uimage2DArray"
		<< "imageCubeArray" << "iimageCubeArray" << "uimageCubeArray"
		<< "imageBuffer"    << "iimageBuffer"    << "uimageBuffer"
		<< "image2DMS"      << "iimage2DMS"      << "uimage2DMS"
		<< "image2DMSArray" << "iimage2DMSArray" << "uimage2DMSArray"
		// Image Formats:
		// Floating-point:  // Signed integer:
		<< "rgba32f"        << "rgba32i"
		<< "rgba16f"        << "rgba16i"
		<< "rg32f"          << "rgba8i"
		<< "rg16f"          << "rg32i"
		<< "r11f_g11f_b10f" << "rg16i"
		<< "r32f"           << "rg8i"
		<< "r16f"           << "r32i"
		<< "rgba16"         << "r16i"
		<< "rgb10_a2"       << "r8i"
		<< "rgba8"          << "r8ui"
		<< "rg16"           // Unsigned integer:
		<< "rg8"            << "rgba32ui"
		<< "r16"            << "rgba16ui"
		<< "r8"             << "rgb10_a2ui"
		<< "rgba16_snorm"   << "rgba8ui"
		<< "rgba8_snorm"    << "rg32ui"
		<< "rg16_snorm"     << "rg16ui"
		<< "rg8_snorm"      << "rg8ui"
		<< "r16_snorm"      << "r32ui"
		<< "r8_snorm"       << "r16ui"
	;
	syntax_highlighter* sh_glsl = new syntax_highlighter(m_glsl_text->document());
	sh_glsl->AddWordRule(glsl_syntax, QColor(Qt::darkBlue));                                  // normal words like: soka, nani, or gomen
	sh_glsl->AddSimpleRule(QStringList("\\bGL_(?:[A-Z]|_)+\\b"), QColor(Qt::darkMagenta));    // constants like: GL_OMAE_WA_MOU_SHINDEIRU
	sh_glsl->AddSimpleRule(QStringList("\\bgl_(?:[A-Z]|[a-z]|_)+\\b"), QColor(Qt::darkCyan)); // reserved types like: gl_exploooooosion
	sh_glsl->AddSimpleRule(QStringList("\\B#[^\\s]+\\b"), QColor(Qt::darkGray));              // preprocessor instructions like: #waifu megumin
	sh_glsl->AddCommentRule("\/\/", QColor(Qt::darkGreen));                                   // comments like: // No comment
	sh_glsl->AddCommentRule("\/\*", QColor(Qt::darkGreen), true, "\*\/");                     // comments like: /* I am trapped! Please help me! */

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

	myMenu.exec(mapToGlobal(pos));
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
		LOG_ERROR(LOADER, "CgDisasm: Failed to open %s", sstr(m_path_last));
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
