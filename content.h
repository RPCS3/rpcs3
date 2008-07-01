#pragma once

#include <ios>
#include "parserstate.h"

namespace YAML
{
	class Scanner;
	class Parser;

	class Content
	{
	public:
		Content();
		virtual ~Content();

		virtual void Parse(Scanner *pScanner, const ParserState& state) = 0;
		virtual void Write(std::ostream& out, int indent) = 0;

	protected:
	};
}
