#include "parser.h"
#include "scanner.h"
#include "token.h"
#include <iostream>

namespace YAML
{
	Parser::Parser(std::istream& in): m_pScanner(0)
	{
		m_pScanner = new Scanner(in);
	}

	Parser::~Parser()
	{
		delete m_pScanner;
	}

	void Parser::GetNextDocument(Document& document)
	{
		document.Parse(m_pScanner);
	}

	void Parser::PrintTokens()
	{
		while(1) {
			Token *pToken = m_pScanner->GetNextToken();
			if(!pToken)
				break;

			std::cout << *pToken << std::endl;
		}
	}
}
