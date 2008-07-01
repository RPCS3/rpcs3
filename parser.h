#pragma once

#include <ios>
#include "document.h"

namespace YAML
{
	class Node;
	class Scanner;

	class Parser
	{
	public:
		Parser(std::istream& in);
		~Parser();

		void GetNextDocument(Document& document);
		void PrintTokens();

	private:
		Scanner *m_pScanner;
	};
}
