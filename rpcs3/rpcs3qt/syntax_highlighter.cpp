#include "syntax_highlighter.h"
#include "qt_utils.h"

Highlighter::Highlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
}

void Highlighter::addRule(const QString &pattern, const QBrush &brush)
{
	HighlightingRule rule;
	rule.pattern = QRegularExpression(pattern);
	rule.format.setForeground(brush);
	highlightingRules.append(rule);
}

void Highlighter::highlightBlock(const QString &text)
{
	for (const HighlightingRule& rule : highlightingRules)
	{
		QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
		while (matchIterator.hasNext())
		{
			QRegularExpressionMatch match = matchIterator.next();
			setFormat(match.capturedStart(), match.capturedLength(), rule.format);
		}
	}
	setCurrentBlockState(0);

	if (commentStartExpression.pattern().isEmpty() || commentEndExpression.pattern().isEmpty())
		return;

	int startIndex = 0;
	if (previousBlockState() != 1)
		startIndex = text.indexOf(commentStartExpression);

	while (startIndex >= 0)
	{
		const QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
		const int endIndex = match.capturedStart();
		int commentLength;

		if (endIndex == -1)
		{
			setCurrentBlockState(1);
			commentLength = text.length() - startIndex;
		}
		else
		{
			commentLength = endIndex - startIndex + match.capturedLength();
		}
		setFormat(startIndex, commentLength, multiLineCommentFormat);
		startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
	}
}

LogHighlighter::LogHighlighter(QTextDocument* parent) : Highlighter(parent)
{
	const QColor color = gui::utils::get_foreground_color();

	//addRule("^[^·].*$", gui::utils::get_label_color("log_level_always", Qt::darkCyan, Qt::cyan)); // unused for now
	addRule("^·F.*$", gui::utils::get_label_color("log_level_fatal", Qt::darkMagenta, Qt::magenta));
	addRule("^·E.*$", gui::utils::get_label_color("log_level_error", Qt::red, Qt::red));
	addRule("^·U.*$", gui::utils::get_label_color("log_level_todo", Qt::darkYellow, Qt::darkYellow));
	addRule("^·S.*$", gui::utils::get_label_color("log_level_success", Qt::darkGreen, Qt::green));
	addRule("^·W.*$", gui::utils::get_label_color("log_level_warning", Qt::darkYellow, Qt::darkYellow));
	addRule("^·!.*$", gui::utils::get_label_color("log_level_notice", color, color));
	addRule("^·T.*$", gui::utils::get_label_color("log_level_trace", color, color));
}

AsmHighlighter::AsmHighlighter(QTextDocument *parent) : Highlighter(parent)
{
	addRule("^\\b[A-Z0-9]+\\b",       Qt::darkBlue);    // Instructions
	addRule("-?R\\d[^,;\\s]*",        Qt::darkRed);     // -R0.*
	addRule("-?H\\d[^,;\\s]*",        Qt::red);         // -H1.*
	addRule("-?v\\[\\d\\]*[^,;\\s]*", Qt::darkCyan);    // -v[xyz].*
	addRule("-?o\\[\\d\\]*[^,;\\s]*", Qt::darkMagenta); // -o[xyz].*
	addRule("-?c\\[\\d\\]*[^,;\\s]*", Qt::darkYellow);  // -c[xyz].*
	addRule("#[^\\n]*",               Qt::darkGreen);   // Single line comment
}

GlslHighlighter::GlslHighlighter(QTextDocument *parent) : Highlighter(parent)
{
	const QStringList keywordPatterns = QStringList()
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
		<< "r8_snorm"       << "r16ui";

	for (const QString& pattern : keywordPatterns)
		addRule("\\b" + pattern + "\\b",   Qt::darkBlue);    // normal words like: soka, nani, or gomen

	addRule("\\bGL_(?:[A-Z]|_)+\\b",       Qt::darkMagenta); // constants like: GL_OMAE_WA_MOU_SHINDEIRU
	addRule("\\bgl_(?:[A-Z]|[a-z]|_)+\\b", Qt::darkCyan);    // reserved types like: gl_exploooooosion
	addRule("\\B#[^\\s]+\\b",              Qt::darkGray);    // preprocessor instructions like: #waifu megumin
	addRule("//[^\\n]*",                   Qt::darkGreen);   // Single line comment

	// Multi line comment
	multiLineCommentFormat.setForeground(Qt::darkGreen);
	commentStartExpression = QRegularExpression("/\\*");
	commentEndExpression = QRegularExpression("\\*/");
}
