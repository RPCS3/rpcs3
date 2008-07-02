#pragma once

#include <ios>
#include "parserstate.h"

namespace YAML
{
	class Node;
	class Scanner;

	class Document
	{
	public:
		Document();
		~Document();

		void Clear();
		void Parse(Scanner *pScanner, const ParserState& state);
		const Node& GetRoot() const;

		friend std::ostream& operator << (std::ostream& out, const Document& doc);

	private:
		Node *m_pRoot;
	};
}
