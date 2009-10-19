#pragma once

#ifndef PARSER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define PARSER_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "node.h"
#include "parserstate.h"
#include "noncopyable.h"
#include <ios>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace YAML
{
	class Scanner;
	struct Token;

	class Parser: private noncopyable
	{
	public:
		Parser();
		Parser(std::istream& in);
		~Parser();

		operator bool() const;

		void Load(std::istream& in);
		bool GetNextDocument(Node& document);
		void PrintTokens(std::ostream& out);

	private:
		void ParseDirectives();
		void HandleDirective(Token *pToken);
		void HandleYamlDirective(Token *pToken);
		void HandleTagDirective(Token *pToken);

	private:
		std::auto_ptr<Scanner> m_pScanner;
		ParserState m_state;
	};
}

#endif // PARSER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
