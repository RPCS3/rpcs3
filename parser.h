#pragma once

#include <ios>
#include <string>
#include <vector>
#include <map>
#include "node.h"
#include "parserstate.h"

namespace YAML
{
	class Node;
	class Scanner;

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
		void HandleDirective(const std::string& name, const std::vector <std::string>& params);
		void HandleYamlDirective(const std::vector <std::string>& params);
		void HandleTagDirective(const std::vector <std::string>& params);

	private:
		Scanner *m_pScanner;
		ParserState m_state;
	};
}
