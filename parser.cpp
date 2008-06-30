#include "parser.h"
#include "node.h"
#include "token.h"

#include <iostream>

namespace YAML
{
	Parser::Parser(std::istream& in): m_scanner(in)
	{
		// eat the stream start token
		// TODO: check?
		Token *pToken = m_scanner.GetNextToken();
	}

	Parser::~Parser()
	{
	}

	void Parser::GetNextDocument(Document& document)
	{
		// scan and output, for now
		while(1) {
			Token *pToken = m_scanner.GetNextToken();
			if(!pToken)
				break;

			std::cout << typeid(*pToken).name() << ": " << *pToken << std::endl;
			delete pToken;
		}
		getchar();
	}
}
