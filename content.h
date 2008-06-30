#pragma once

#include <ios>

namespace YAML
{
	class Scanner;

	class Content
	{
	public:
		Content();
		virtual ~Content();

		virtual void Parse(Scanner *pScanner) = 0;
		virtual void Write(std::ostream& out, int indent) = 0;

	protected:
	};
}
