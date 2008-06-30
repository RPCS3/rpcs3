#include "parser.h"
#include "scanner.h"

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
}
