#pragma once

#include "ostream.h"
#include <string>

namespace YAML
{
	namespace Utils
	{
		bool WriteString(ostream& out, const std::string& str, bool inFlow);
		bool WriteSingleQuotedString(ostream& out, const std::string& str);
		bool WriteDoubleQuotedString(ostream& out, const std::string& str);
		bool WriteLiteralString(ostream& out, const std::string& str, int indent);
		bool WriteComment(ostream& out, const std::string& str, int postCommentIndent);
		bool WriteAlias(ostream& out, const std::string& str);
		bool WriteAnchor(ostream& out, const std::string& str);
	}
}
