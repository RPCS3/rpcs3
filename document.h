#pragma once

#include <ios>

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
		void Parse(Scanner *pScanner);

		friend std::ostream& operator << (std::ostream& out, const Document& doc);

	private:
		Node *m_pRoot;
	};
}
