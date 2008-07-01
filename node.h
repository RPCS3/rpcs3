#pragma once

#include <string>
#include <ios>
#include "parserstate.h"

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
		void Parse(Scanner *pScanner, const ParserState& state);
		void Write(std::ostream& out, int indent);

	private:
		void ParseHeader(Scanner *pScanner, const ParserState& state);
		void ParseTag(Scanner *pScanner, const ParserState& state);
		void ParseAnchor(Scanner *pScanner, const ParserState& state);
		void ParseAlias(Scanner *pScanner, const ParserState& state);

	private:
		bool m_alias;
		std::string m_anchor, m_tag;
		Content *m_pContent;
	};
}
