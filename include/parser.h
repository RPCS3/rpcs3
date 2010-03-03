#pragma once

#ifndef PARSER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define PARSER_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "node.h"
#include "noncopyable.h"
#include <ios>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace YAML
{
	class Scanner;
	struct ParserState;
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
		void HandleDirective(const Token& token);
		void HandleYamlDirective(const Token& token);
		void HandleTagDirective(const Token& token);

	private:
		std::auto_ptr<Scanner> m_pScanner;
		std::auto_ptr<ParserState> m_pState;
	};
}

#endif // PARSER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
