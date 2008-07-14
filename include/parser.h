#pragma once

#include <ios>
#include <string>
#include <vector>
#include <map>
#include "node.h"
#include "parserstate.h"

namespace YAML
{
	class Scanner;
	struct Token;

	class Parser
	{
	public:
		Parser(std::istream& in);
		~Parser();

		operator bool() const;

		void Load(std::istream& in);
		void GetNextDocument(Node& document);
		void PrintTokens(std::ostream& out);

	private:
		void ParseDirectives();
		void HandleDirective(Token *pToken);
		void HandleYamlDirective(Token *pToken);
		void HandleTagDirective(Token *pToken);

	private:
		Scanner *m_pScanner;
		ParserState m_state;
	};
}
