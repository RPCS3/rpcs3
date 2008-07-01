#pragma once

#include <string>
#include <ios>

namespace YAML
{
	const std::string StrTag = "!!str";
	const std::string SeqTag = "!!seq";
	const std::string MapTag = "!!map";

	class Content;
	class Scanner;

	class Node
	{
	public:
		Node();
		~Node();

		void Clear();
		void Parse(Scanner *pScanner);
		void Write(std::ostream& out, int indent);

	private:
		void ParseHeader(Scanner *pScanner);
		void ParseTag(Scanner *pScanner);
		void ParseAnchor(Scanner *pScanner);
		void ParseAlias(Scanner *pScanner);

	private:
		bool m_alias;
		std::string m_anchor, m_tag;
		Content *m_pContent;
	};
}
